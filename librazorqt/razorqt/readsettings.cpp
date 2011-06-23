#include <QDir>
#include <QtDebug>
#include "readsettings.h"

#define RAZOR_HOME_CFG QDir::homePath() + "/.razor/"

ReadSettings::ReadSettings(const QString & module, QObject * parent)
    : QObject(parent),
      m_module(module)
{

    if (!checkConfigDir())
    {
        qDebug() << "Cannot create user config dir";
        Q_ASSERT(0);
    }

    QString homeFile(RAZOR_HOME_CFG + module + ".conf");

    if (! QFile::exists(homeFile))
    {
        QString path(getSysPath(module + ".conf"));
        if (path.isEmpty())
        {
            QFile f(homeFile);
            if (! f.open(QIODevice::WriteOnly))
            {
                qDebug() << "Cannot open file" << path << "err:" << f.error();
                Q_ASSERT(0);
            }
            f.close();
        }
        else
        {
            // Create parent directories
            QDir homeFileDir(QFileInfo(homeFile).absoluteDir());

            if (!homeFileDir.exists())
                homeFileDir.mkpath(".");


            QFile f(path);

            if (! f.copy(homeFile))
            {
                qDebug() << "Cannot copy file from:" << path << "to:" << homeFileDir.absolutePath();
                Q_ASSERT(0);
            }
            qDebug() << "Copied file:" << path << "into" << homeFileDir.absolutePath();
        }
    }
    else
        qDebug() << "Config found in home dir:" << homeFile;

    m_settings = new QSettings(homeFile, QSettings::IniFormat, this);
}

QSettings* ReadSettings::settings()
{
    if (! m_settings)
        qDebug() << __FILE__ << ":" << __LINE__ << " ReadSettings::settings() settings instance is not initialized.";
    return m_settings;
}

/**
 * @brief this looks in all the usual system paths for our file
 */
QString ReadSettings::getSysPath(const QString & fileName)
{
    QString pathTemplate("%1/%2");
    QString test;

    // a hack to simplify the multiple file location tests
#define TESTFILE(prefix)                            \
    qDebug() << "searching in" << (prefix) << fileName; \
    test = pathTemplate.arg( (prefix), fileName );  \
    qDebug() << "    test " << test; \
    if (QFile::exists(test))                        \
    {                                               \
        qDebug() << "Readsettings: Config found in" << (prefix) << fileName; \
        return test;                                \
    }

    TESTFILE(RAZOR_HOME_CFG);

#ifdef SHARE_DIR
    // test for non-standard install dirs - useful for development for example
    TESTFILE(SHARE_DIR)
#else
#warning SHARE_DIR is not defined. Config will not be searched in the CMAKE_INSTALL_PREFIX
    // this is ugly and it should be used only in a very special cases
    // standard cmake build contains the SHARE_DIR defined in the top level CMakeLists.txt
    //
    // /usr/local/ goes first as it's usually before the /usr in the PATH etc.
    TESTFILE("/usr/local/share/razor/");
    TESTFILE("/usr/share/razor/")
#endif

    // die silently
    //! \todo TODO/FIXME: shouldn't be here something like "wraning and close"?
    qDebug() << "ReadSettings: could not find file " << fileName;
    return "";
}

bool ReadSettings::checkConfigDir()
{
    if ( !QFile::exists(RAZOR_HOME_CFG))
    {
        QDir d(QDir::home());
        return d.mkdir(".razor");
    }
    return true;
}


ReadTheme::ReadTheme(const QString & name, QObject * parent)
    : QObject(parent)
{
    qDebug() << "Reading Theme ____________________________________________";
    QString path(ReadSettings::getSysPath("themes/" + name));
    if (path.isEmpty())
    {
        qDebug() << "Theme" << name << "cannot be found in any location";
        return;
    }

    QString indexName(path + "/index.theme");
    qDebug() << "Theme:" << indexName;
    QSettings s(indexName, QSettings::IniFormat, this);
    // There is something strange... If I remove this qDebug the wallpapers array is not found...
    qDebug() << s.childKeys() << s.childGroups();

    int cnt = s.beginReadArray("wallpapers");
    for (int i = 0; i < cnt; ++i)
    {
        qDebug() << "WALL";
        s.setArrayIndex(i);
        QString pth(s.value("file", "").toString());
        if (pth.isEmpty())
        {
            qDebug() << "Theme: wallpaper SKIPPED" << i << m_desktopBackgrounds[i];
            continue;
        }
        m_desktopBackgrounds[i] = path + "/" + pth;
        qDebug() << "Theme: wallpaper" << i << m_desktopBackgrounds[i];
    }
    s.endArray();

    m_qssPath = s.value("stylesheet", "").toString();
    if ( !m_qssPath.isEmpty())
    {
        m_qssPath = path + "/" + m_qssPath;
        if (!QFile::exists(m_qssPath))
        {
            qDebug() << "Theme: file does not exist" << m_qssPath;
            return;
        }

        qDebug() << "Theme: Reading QSS from" << m_qssPath;
        QFile f(m_qssPath);
        if (! f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Theme: Canot open file for reading:" << m_qssPath;
            return;
        }
        m_qss = f.readAll();
        f.close();
        // handle relative paths
        if (! m_qss.isEmpty())
        {
            qDebug() << "Theme: replace paths";
            QString dirName = QFileInfo(m_qssPath).canonicalPath();
            m_qss.replace(QRegExp("url.[ \\t\\s]*", Qt::CaseInsensitive, QRegExp::RegExp2), "url(" + dirName + "/");
        }
    }
    else
        qDebug() << "Theme: QSS is not defined in the style";

//    s.beginGroup("icons");
//    foreach (QString key, s.childKeys())
//    {
//        m_icons[key] = path + "/" + s.value(key, "").toString();
//        qDebug() << "Theme: icon" << key << "=" << m_icons[key];
//    }
//    s.endGroup();
    qDebug() << "Reading Theme End ________________________________________";
}

QString ReadTheme::desktopBackground(int screen)
{
    if (m_desktopBackgrounds.contains(screen))
        return m_desktopBackgrounds[screen];
    
    if (m_desktopBackgrounds.count())
        return m_desktopBackgrounds.values().at(0);
    
    return QString();
}
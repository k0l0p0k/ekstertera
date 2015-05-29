#include "icon.h"
//----------------------------------------------------------------------------------------------
#include "pool.h"
#include "tasks/all.h"
//----------------------------------------------------------------------------------------------
static EteraIconProvider* g_icon_provider = NULL;
//----------------------------------------------------------------------------------------------

void EteraIconProvider::init()
{
    g_icon_provider = new EteraIconProvider();
}
//----------------------------------------------------------------------------------------------

void EteraIconProvider::cleanup()
{
    delete g_icon_provider;
}
//----------------------------------------------------------------------------------------------

EteraIconProvider* EteraIconProvider::instance()
{
    return g_icon_provider;
}
//----------------------------------------------------------------------------------------------

EteraIconProvider::EteraIconProvider() : QObject()
{
    m_icon_sizes << 32 << 48 << 64 << 96 << 128 << 256;
    m_default_icon_size_index = 1 /* 48 */;

#ifdef ETERA_WS_WIN
    // данные типы файлов не имеют больших иконок под windows
    m_jumbo_workaround << "cer" << "chm" << "css" << "js" << "msi" << "prf" << "py" << "sh" << "wsf";
#endif

    // публичность
    m_link = prepareIcon(QIcon::fromTheme("emblem-symbolic-link", QIcon(":/icons/tango/emblem-symbolic-link.svg")), 2);

    // директория
    m_dir      = prepareIcon(QIcon::fromTheme("folder", QIcon(":/icons/tango/folder.svg")));
    m_dir_link = addLinkIcon(m_dir);

    // файл
    m_file      = prepareIcon(QIcon::fromTheme("text-x-generic", QIcon(":/icons/tango/text-x-generic.svg")));
    m_file_link = addLinkIcon(m_file);

    /*!
     * \brief Описатель соответствий типа медиа и иконки
     */
    typedef struct {
        EteraItemMediaType Type;           /*!< \brief Тип медиа              */
        const char*        ThemeIcon;      /*!< \brief Имя иконки из темы     */
        const char*        ResourceIcon;   /*!< \brief Имя иконки из ресурсов */
    } EteraMediaIconMap;

    /*!
     * \brief Карта соответствий типа медиа и иконки
     */
    const EteraMediaIconMap media_icon_map[] = {
        { eimtAudio,       "audio-x-generic",          ":/icons/tango/audio-x-generic.svg"          },
        { eimtBackup,      "package-x-generic",        ":/icons/tango/package-x-generic.svg"        },
        { eimtBook,        NULL,                       NULL                                         },
        { eimtCompressed,  "package-x-generic",        ":/icons/tango/package-x-generic.svg"        },
        { eimtData,        NULL,                       NULL                                         },
        { eimtDevelopment, "text-x-script",            ":/icons/tango/text-x-script.svg"            },
        { eimtDiskimage,   "package-x-generic",        ":/icons/tango/package-x-generic.svg"        },
        { eimtDocument,    "x-office-document",        ":/icons/tango/x-office-document.svg"        },
        { eimtEncoded,     NULL,                       NULL                                         },
        { eimtExecutable,  "application-x-executable", ":/icons/tango/application-x-executable.svg" },
        { eimtFlash,       "video-x-generic",          ":/icons/tango/video-x-generic.svg"          },
        { eimtFont,        "font-x-generic",           ":/icons/tango/font-x-generic.svg"           },
        { eimtImage,       "image-x-generic",          ":/icons/tango/image-x-generic.svg"          },
        { eimtSettings,    NULL,                       NULL                                         },
        { eimtSpreadsheet, "x-office-spreadsheet",     ":/icons/tango/x-office-spreadsheet.svg"     },
        { eimtText,        "text-x-generic",           ":/icons/tango/text-x-generic.svg"           },
        { eimtVideo,       "video-x-generic",          ":/icons/tango/video-x-generic.svg"          },
        { eimtWeb,         "text-html",                ":/icons/tango/text-html.svg"                },
        { eimtUnknown,     NULL,                       NULL                                         }
    };

    const EteraMediaIconMap* map = media_icon_map;

    while (map->Type != eimtUnknown) {
        if (map->ThemeIcon != NULL || map->ResourceIcon != NULL) {
            QIcon icon_base;

            if (map->ThemeIcon != NULL)
                icon_base = QIcon::fromTheme(map->ThemeIcon, QIcon(map->ResourceIcon));
            else
                icon_base = QIcon(map->ResourceIcon);

            icon_base = prepareIcon(icon_base);

            QIcon icon_link = addLinkIcon(icon_base);

            m_media_icon[map->Type]      = icon_base;
            m_media_icon_link[map->Type] = icon_link;
        }

        map++;
    }
}
//----------------------------------------------------------------------------------------------

EteraIconProvider::~EteraIconProvider()
{
}
//----------------------------------------------------------------------------------------------

QIcon EteraIconProvider::prepareIcon(const QIcon& icon, int scale, bool center)
{
    QIcon result;

    for (int i = 0; i < m_icon_sizes.count(); i++) {
        int size = m_icon_sizes[i] / scale;

        QPixmap pixmap = icon.pixmap(size);
        if (pixmap.width() < size) {
            if (center == false)
                pixmap = pixmap.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            else {
                int dx = (size - pixmap.width()) / 2;

                QPixmap base_pixmap(size, size);
                base_pixmap.fill(Qt::transparent);

                QPainter painter(&base_pixmap);

                painter.drawPixmap(dx, dx, pixmap.width(), pixmap.width(), pixmap);

                pixmap = base_pixmap;
            }
        }

        result.addPixmap(pixmap);
    }

    return result;
}
//----------------------------------------------------------------------------------------------

QIcon EteraIconProvider::addLinkIcon(const QIcon& base_icon)
{
    QIcon result;

    for (int i = 0; i < m_icon_sizes.count(); i++) {
        int size = m_icon_sizes[i];
        int link_size = size / 2;

        QPixmap base_pixmap = base_icon.pixmap(size);
        QPixmap link_pixmap = m_link.pixmap(link_size);

        QPainter painter(&base_pixmap);

        painter.drawPixmap(link_size, link_size, link_pixmap);

        result.addPixmap(base_pixmap);
    }

    return result;
}
//----------------------------------------------------------------------------------------------
#ifdef ETERA_WS_X11_OR_WIN
bool EteraIconProvider::cachedIcon(QIcon& icon, const QString& key, bool shared)
{
    if (m_cache_icon_miss.contains(key) == true)
        return false;

    if (shared == true) {
        if (m_cache_icon_link.contains(key) == true) {
            icon = m_cache_icon_link[key];
            return true;
        }

        // непубличная иконка могла уже быть закэширована
        if (m_cache_icon.contains(key) == true) {
            QIcon link_icon = addLinkIcon(m_cache_icon[key]);
            m_cache_icon_link[key] = link_icon;
            icon = link_icon;
            return true;
        }

        return false;
    }

    if (m_cache_icon.contains(key) == true) {
        icon = m_cache_icon[key];
        return true;
    }

    return false;
}
#endif
//----------------------------------------------------------------------------------------------
#ifdef ETERA_WS_X11_OR_WIN
bool EteraIconProvider::cacheIcon(QIcon& icon, const QIcon& base_icon, const QString& key, bool shared)
{
    m_cache_icon[key] = base_icon;

    if (shared == true) {
        QIcon link_icon = addLinkIcon(base_icon);
        m_cache_icon_link[key] = link_icon;
        icon = link_icon;
    } else
        icon = base_icon;

    return true;
}
#endif
//----------------------------------------------------------------------------------------------
#ifdef ETERA_WS_WIN
bool EteraIconProvider::extensionIcon(QIcon& icon, const QString& ext, bool shared)
{
    if (cachedIcon(icon, ext, shared) == true)
        return true;

    // получение индекса иконки по расширению
    SHFILEINFO sfi;
    if (SHGetFileInfo((LPCTSTR)("." + ext).utf16(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES) == 0)
        return false;

    if (sfi.iIcon == 0) {
        m_cache_icon_miss.insert(ext);
        return false;
    }

    // генерация иконки по размерам
    QIcon base_icon;

    QList<int> sizes;
    sizes << SHIL_LARGE << SHIL_EXTRALARGE;

    bool center = m_jumbo_workaround.contains(ext);
    if (center == false)
        sizes << SHIL_JUMBO;

    for (int i = 0; i < sizes.size(); i++) {
        int size = sizes[i];

        IImageList* ilist;
        if (FAILED(SHGetImageList(size, IID_IImageList, (void**)&ilist)))
            return false;

        HICON   hicon;
        HRESULT result = ilist->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hicon);

        ilist->Release();

        if (result != S_OK)
            return false;

        QPixmap pixmap = QPixmap::fromWinHICON(hicon);

        DestroyIcon(hicon);

        if (pixmap.isNull() == true)
            return false;

        base_icon.addPixmap(pixmap);
    }

    return cacheIcon(icon, prepareIcon(base_icon, 1, center), ext, shared);
}
#endif
//----------------------------------------------------------------------------------------------
#ifdef ETERA_WS_X11
bool EteraIconProvider::mimeIcon(QIcon& icon, const QString& mime, bool shared)
{
    if (cachedIcon(icon, mime, shared) == true)
        return true;

    QMimeType mime_type = QMimeDatabase().mimeTypeForName(mime);

    if (mime_type.isValid() == false || mime_type.isDefault() == true) {
        m_cache_icon_miss.insert(mime);
        return false;
    }

    QIcon base_icon;
    if (QIcon::hasThemeIcon(mime_type.iconName()) == true)
        base_icon = QIcon::fromTheme(mime_type.iconName());
    else if (QIcon::hasThemeIcon(mime_type.genericIconName()) == true)
        base_icon = QIcon::fromTheme(mime_type.genericIconName());
    else {
        m_cache_icon_miss.insert(mime);
        return false;
    }

    return cacheIcon(icon, prepareIcon(base_icon), mime, shared);
}
#endif
//----------------------------------------------------------------------------------------------

bool EteraIconProvider::mediaIcon(QIcon& icon, EteraItemMediaType type, bool shared)
{
    if (shared == true) {
        if (m_media_icon_link.contains(type) == false)
            return false;

        icon = m_media_icon_link[type];

        return true;
    }

    if (m_media_icon.contains(type) == false)
        return false;

    icon = m_media_icon[type];

    return true;
}
//----------------------------------------------------------------------------------------------

QIcon EteraIconProvider::icon(const EteraItem* item)
{
    if (item->isDir() == true)
        return (item->isPublic() == true ? m_dir_link : m_dir);
    else if (item->isFile() == true) {
        QIcon result;

#ifdef ETERA_WS_WIN
        if (extensionIcon(result, item->extension(), item->isPublic()) == true)
            return result;
#endif

#ifdef ETERA_WS_X11
        if (mimeIcon(result, item->mimeType(), item->isPublic()) == true)
            return result;
#endif

        if (mediaIcon(result, item->mediaType(), item->isPublic()) == true)
            return result;

        return (item->isPublic() == true ? m_file_link : m_file);
    }

    return QIcon();
}
//----------------------------------------------------------------------------------------------

bool EteraIconProvider::preview(WidgetDiskItem* item)
{
    const EteraItem* eitem = item->item();

    QString preview = eitem->preview();

    if (eitem->isPublic() == true) {
        if (m_preview_cache_link.contains(preview) == true) {
            item->setIcon(m_preview_cache_link[preview]);
            return true;
        }

        if (m_preview_cache.contains(preview) == true) {
            QIcon link_icon = addLinkIcon(m_preview_cache[preview]);
            m_preview_cache_link[preview] = link_icon;
            item->setIcon(link_icon);
            return true;
        }
    } else if (m_preview_cache.contains(preview) == true) {
        item->setIcon(m_preview_cache[preview]);
        return true;
    }

    item->setIcon(icon(eitem));

    m_preview_wait[preview] = item;

    EteraTaskGET* get = new EteraTaskGET(preview, "");

    connect(get, SIGNAL(onSuccess(quint64, const QVariantMap&)), this, SLOT(task_on_get_preview_success(quint64, const QVariantMap&)));
    connect(get, SIGNAL(onError(quint64, int, const QString&, const QVariantMap&)), this, SLOT(task_on_get_preview_error(quint64, int, const QString&, const QVariantMap&)));

    EteraThreadPool::globalInstance()->start(get);

    return false;
}
//----------------------------------------------------------------------------------------------

void EteraIconProvider::cancelPreview(const WidgetDiskItem* item)
{
    QString preview = item->item()->preview();
    if (m_preview_wait.contains(preview) == true)
        m_preview_wait.remove(preview);
}
//----------------------------------------------------------------------------------------------

void EteraIconProvider::task_on_get_preview_success(quint64 /*id*/, const QVariantMap& args)
{
    QString    source = args["source"].toString();
    QByteArray target = args["target"].toByteArray();

    QPixmap pixmap;
    pixmap.loadFromData(target);

    if (pixmap.isNull() == true) {
        m_preview_wait.remove(source);
        return;
    }

    QIcon icon;
    icon.addPixmap(pixmap);

    icon = prepareIcon(icon);

    m_preview_cache[source] = icon;

    WidgetDiskItem* item = m_preview_wait.value(source, NULL);
    if (item != NULL) {
        m_preview_wait.remove(source);

        if (item->item()->isPublic() == false)
            item->setIcon(icon);
        else {
            QIcon link_icon = addLinkIcon(icon);
            m_preview_cache_link[source] = link_icon;
            item->setIcon(link_icon);
        }
    }
}
//----------------------------------------------------------------------------------------------

void EteraIconProvider::task_on_get_preview_error(quint64 /*id*/, int /*code*/, const QString& /*error*/, const QVariantMap& args)
{
    QString source = args["source"].toString();
    m_preview_wait.remove(source);
}
//----------------------------------------------------------------------------------------------

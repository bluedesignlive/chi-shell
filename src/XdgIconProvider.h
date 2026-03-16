#ifndef XDGICONPROVIDER_H
#define XDGICONPROVIDER_H

#include <QQuickImageProvider>
#include <QIcon>

class XdgIconProvider : public QQuickImageProvider
{
public:
    XdgIconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size,
                          const QSize &requestedSize) override
    {
        int w = qMax(1, requestedSize.width()  > 0 ? requestedSize.width()  : 48);
        int h = qMax(1, requestedSize.height() > 0 ? requestedSize.height() : 48);

        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull())
            icon = QIcon::fromTheme(id.toLower());

        if (icon.isNull()) {
            if (size) *size = QSize();
            return QPixmap();           // Image.status → Error → fallback text shows
        }

        QPixmap pix = icon.pixmap(w, h);
        if (size) *size = pix.size();
        return pix;
    }
};

#endif // XDGICONPROVIDER_H

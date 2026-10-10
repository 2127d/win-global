// SPDX-License-Identifier: MIT
// Route only Merkuro Calendar's embedded QML to its version-checked local copy.
#include <QQmlApplicationEngine>
#include <QQmlAbstractUrlInterceptor>
#include <QFileInfo>
#include <QUrl>
#include <dlfcn.h>
#include <cstdlib>

class MerkuroUrls final : public QQmlAbstractUrlInterceptor {
public:
    explicit MerkuroUrls(QString directory) : directory(std::move(directory)) {}
    QUrl intercept(const QUrl &url, DataType) override {
        const QString prefix = QStringLiteral("/qt/qml/org/kde/merkuro/calendar/");
        if (url.scheme() == QStringLiteral("qrc") && url.path().startsWith(prefix)) {
            const auto file = directory + QLatin1Char('/') + url.path().mid(prefix.size());
            if (QFileInfo::exists(file)) return QUrl::fromLocalFile(file);
        }
        return url;
    }
private:
    const QString directory;
};

void QQmlApplicationEngine::loadFromModule(QAnyStringView uri, QAnyStringView typeName)
{
    using Load = void (*)(QQmlApplicationEngine *, QAnyStringView, QAnyStringView);
    static auto original = reinterpret_cast<Load>(dlsym(RTLD_NEXT, "_ZN21QQmlApplicationEngine14loadFromModuleE14QAnyStringViewS0_"));
    if (!original) std::abort();
    const auto directory = qEnvironmentVariable("SEQUOIA_MERKURO_QML");
    if (!directory.isEmpty() && uri.toString() == QStringLiteral("org.kde.merkuro.calendar")) {
        auto interceptor = new MerkuroUrls(directory);
        addUrlInterceptor(interceptor);
        connect(this, &QObject::destroyed, [interceptor] { delete interceptor; });
    }
    original(this, uri, typeName);
}

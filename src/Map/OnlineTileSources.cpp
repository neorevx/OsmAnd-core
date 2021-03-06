#include "OnlineTileSources.h"
#include "OnlineTileSources_P.h"

#include <QFile>

OsmAnd::OnlineTileSources::OnlineTileSources()
    : _p(new OnlineTileSources_P(this))
{
}

OsmAnd::OnlineTileSources::~OnlineTileSources()
{
}

bool OsmAnd::OnlineTileSources::loadFrom(const QByteArray& content)
{
    return _p->loadFrom(content);
}

bool OsmAnd::OnlineTileSources::loadFrom(QIODevice& ioDevice)
{
    return _p->loadFrom(ioDevice);
}

bool OsmAnd::OnlineTileSources::loadFrom(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const auto success = loadFrom(file);
    file.close();
    return success;
}

bool OsmAnd::OnlineTileSources::saveTo(QIODevice& ioDevice) const
{
    return _p->saveTo(ioDevice);
}

bool OsmAnd::OnlineTileSources::saveTo(const QString& fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    const auto success = saveTo(file);
    if (!success)
    {
        file.close();
        file.remove();
    }
    return success;
}

QHash< QString, std::shared_ptr<const OsmAnd::OnlineTileSources::Source> > OsmAnd::OnlineTileSources::getCollection() const
{
    return _p->getCollection();
}

std::shared_ptr<const OsmAnd::OnlineTileSources::Source> OsmAnd::OnlineTileSources::getSourceByName(const QString& sourceName) const
{
    return _p->getSourceByName(sourceName);
}

bool OsmAnd::OnlineTileSources::addSource(const std::shared_ptr<Source>& source)
{
    return _p->addSource(source);
}

bool OsmAnd::OnlineTileSources::removeSource(const QString& sourceName)
{
    return _p->removeSource(sourceName);
}

const QString OsmAnd::OnlineTileSources::BuiltInOsmAndHD(QLatin1String("osmand_hd"));

std::shared_ptr<const OsmAnd::OnlineTileSources> OsmAnd::OnlineTileSources::getBuiltIn()
{
    return OnlineTileSources_P::getBuiltIn();
}

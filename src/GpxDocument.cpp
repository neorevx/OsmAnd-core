#include "GpxDocument.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QFile>
#include <QStack>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "Common.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::GpxDocument::GpxDocument()
{
}

OsmAnd::GpxDocument::~GpxDocument()
{
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::createFrom(const std::shared_ptr<const GeoInfoDocument>& document)
{
    return nullptr;
}

bool OsmAnd::GpxDocument::saveTo(QXmlStreamWriter& xmlWriter) const
{
    xmlWriter.writeStartDocument(QLatin1String("1.0"), true);

    //<gpx
    //	  version="1.1"
    //	  creator="OsmAnd"
    //	  xmlns="http://www.topografix.com/GPX/1/1"
    //	  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    //	  xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
    xmlWriter.writeStartElement(QLatin1String("gpx"));
    xmlWriter.writeAttribute(QLatin1String("version"), version.isEmpty() ? QLatin1String("1.1") : version);
    xmlWriter.writeAttribute(QLatin1String("creator"), creator.isEmpty() ? QLatin1String("OsmAnd Core") : creator);
    xmlWriter.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://www.topografix.com/GPX/1/1"));
    xmlWriter.writeAttribute(QLatin1String("xmlns:xsi"), QLatin1String("http://www.w3.org/2001/XMLSchema-instance"));
    xmlWriter.writeAttribute(QLatin1String("xsi:schemaLocation"), QLatin1String("http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"));

    // Write metadata (if present)
    if (metadata)
    {
        // <metadata>
        xmlWriter.writeStartElement(QLatin1String("metadata"));

        // <name>
        if (!metadata->name.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("name"), metadata->name);

        // <desc>
        if (!metadata->description.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("desc"), metadata->description);

        // Links
        if (!metadata->links.isEmpty())
            writeLinks(metadata->links, xmlWriter);

        // <time>
        if (!metadata->timestamp.isNull())
            xmlWriter.writeTextElement(QLatin1String("time"), metadata->timestamp.toString(QLatin1String("YYYY-MM-DDThh:mm:ss")));
        
        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(metadata->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // </metadata>
        xmlWriter.writeEndElement();
    }

    // <wpt>'s
    for (const auto& locationMark : constOf(locationMarks))
    {
        // <wpt>
        xmlWriter.writeStartElement(QLatin1String("wpt"));
        xmlWriter.writeAttribute(QLatin1String("lat"), QString::number(locationMark->position.latitude, 'f', 12));
        xmlWriter.writeAttribute(QLatin1String("lon"), QString::number(locationMark->position.longitude, 'f', 12));

        // <name>
        if (!locationMark->name.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("name"), locationMark->name);

        // <desc>
        if (!locationMark->description.isEmpty())
        xmlWriter.writeTextElement(QLatin1String("desc"), locationMark->description);

        // <ele>
        if (!qIsNaN(locationMark->elevation))
            xmlWriter.writeTextElement(QLatin1String("ele"), QString::number(locationMark->elevation, 'f', 12));

        // <time>
        if (!locationMark->timestamp.isNull())
            xmlWriter.writeTextElement(QLatin1String("time"), locationMark->timestamp.toString(QLatin1String("YYYY-MM-DDThh:mm:ss")));

        // <cmt>
        if (!locationMark->comment.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("cmt"), locationMark->comment);

        // <type>
        if (!locationMark->type.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("type"), locationMark->type);

        // Links
        if (!locationMark->links.isEmpty())
            writeLinks(locationMark->links, xmlWriter);

        if (const auto wpt = std::dynamic_pointer_cast<const GpxWpt>(locationMark.shared_ptr()))
        {
            // <magvar>
            if (!qIsNaN(wpt->magneticVariation))
                xmlWriter.writeTextElement(QLatin1String("magvar"), QString::number(wpt->magneticVariation, 'f', 12));

            // <geoidheight>
            if (!qIsNaN(wpt->geoidHeight))
                xmlWriter.writeTextElement(QLatin1String("geoidheight"), QString::number(wpt->geoidHeight, 'f', 12));

            // <src>
            if (!wpt->source.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("src"), wpt->source);

            // <sym>
            if (!wpt->symbol.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("sym"), wpt->symbol);

            // <fix>
            switch (wpt->fixType)
            {
                case GpxFixType::None:
                    xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("none"));
                    break;

                case GpxFixType::PositionOnly:
                    xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("2d"));
                    break;

                case GpxFixType::PositionAndElevation:
                    xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("3d"));
                    break;

                case GpxFixType::DGPS:
                    xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("dgps"));
                    break;

                case GpxFixType::PPS:
                    xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("pps"));
                    break;

                case GpxFixType::Unknown:
                default:
                    break;
            }

            // <sat>
            if (wpt->satellitesUsedForFixCalculation >= 0)
                xmlWriter.writeTextElement(QLatin1String("sat"), QString::number(wpt->satellitesUsedForFixCalculation));

            // <hdop>
            if (!qIsNaN(wpt->horizontalDilutionOfPrecision))
                xmlWriter.writeTextElement(QLatin1String("hdop"), QString::number(wpt->horizontalDilutionOfPrecision, 'f', 12));

            // <vdop>
            if (!qIsNaN(wpt->verticalDilutionOfPrecision))
                xmlWriter.writeTextElement(QLatin1String("vdop"), QString::number(wpt->verticalDilutionOfPrecision, 'f', 12));

            // <pdop>
            if (!qIsNaN(wpt->positionDilutionOfPrecision))
                xmlWriter.writeTextElement(QLatin1String("pdop"), QString::number(wpt->positionDilutionOfPrecision, 'f', 12));

            // <ageofdgpsdata>
            if (!qIsNaN(wpt->ageOfGpsData))
                xmlWriter.writeTextElement(QLatin1String("ageofdgpsdata"), QString::number(wpt->ageOfGpsData, 'f', 12));

            // <dgpsid>
            if (wpt->dgpsStationId >= 0)
                xmlWriter.writeTextElement(QLatin1String("dgpsid"), QString::number(wpt->dgpsStationId));
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(locationMark->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // </wpt>
        xmlWriter.writeEndElement();
    }

    // <trk>'s
    for (const auto& track : constOf(tracks))
    {
        // <trk>
        xmlWriter.writeStartElement(QLatin1String("trk"));
        
        // <name>
        if (!track->name.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("name"), track->name);

        // <desc>
        if (!track->description.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("desc"), track->description);

        // <cmt>
        if (!track->comment.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("cmt"), track->comment);

        // <type>
        if (!track->type.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("type"), track->type);

        // Links
        if (!track->links.isEmpty())
            writeLinks(track->links, xmlWriter);

        if (const auto trk = std::dynamic_pointer_cast<const GpxTrk>(track.shared_ptr()))
        {
            // <src>
            if (!trk->source.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("src"), trk->source);

            // <number>
            if (trk->slotNumber >= 0)
                xmlWriter.writeTextElement(QLatin1String("sat"), QString::number(trk->slotNumber));
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(track->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // Write track segments
        for (const auto& trackSegment : constOf(track->segments))
        {
            // <trkseg>
            xmlWriter.writeStartElement(QLatin1String("trkseg"));

            // Write track points
            for (const auto& trackPoint : constOf(trackSegment->points))
            {
                // <trkpt>
                xmlWriter.writeStartElement(QLatin1String("trkpt"));
                xmlWriter.writeAttribute(QLatin1String("lat"), QString::number(trackPoint->position.latitude, 'f', 12));
                xmlWriter.writeAttribute(QLatin1String("lon"), QString::number(trackPoint->position.longitude, 'f', 12));

                // <name>
                if (!trackPoint->name.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("name"), trackPoint->name);

                // <desc>
                if (!trackPoint->description.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("desc"), trackPoint->description);

                // <ele>
                if (!qIsNaN(trackPoint->elevation))
                    xmlWriter.writeTextElement(QLatin1String("ele"), QString::number(trackPoint->elevation, 'f', 12));

                // <time>
                if (!trackPoint->timestamp.isNull())
                    xmlWriter.writeTextElement(QLatin1String("time"), trackPoint->timestamp.toString(QLatin1String("YYYY-MM-DDThh:mm:ss")));

                // <cmt>
                if (!trackPoint->comment.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("cmt"), trackPoint->comment);

                // <type>
                if (!trackPoint->type.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("type"), trackPoint->type);

                // Links
                if (!trackPoint->links.isEmpty())
                    writeLinks(trackPoint->links, xmlWriter);

                if (const auto trkpt = std::dynamic_pointer_cast<const GpxTrkPt>(trackPoint.shared_ptr()))
                {
                    // <magvar>
                    if (!qIsNaN(trkpt->magneticVariation))
                        xmlWriter.writeTextElement(QLatin1String("magvar"), QString::number(trkpt->magneticVariation, 'f', 12));

                    // <geoidheight>
                    if (!qIsNaN(trkpt->geoidHeight))
                        xmlWriter.writeTextElement(QLatin1String("geoidheight"), QString::number(trkpt->geoidHeight, 'f', 12));

                    // <src>
                    if (!trkpt->source.isEmpty())
                        xmlWriter.writeTextElement(QLatin1String("src"), trkpt->source);

                    // <sym>
                    if (!trkpt->symbol.isEmpty())
                        xmlWriter.writeTextElement(QLatin1String("sym"), trkpt->symbol);

                    // <fix>
                    switch (trkpt->fixType)
                    {
                        case GpxFixType::None:
                            xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("none"));
                            break;

                        case GpxFixType::PositionOnly:
                            xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("2d"));
                            break;

                        case GpxFixType::PositionAndElevation:
                            xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("3d"));
                            break;

                        case GpxFixType::DGPS:
                            xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("dgps"));
                            break;

                        case GpxFixType::PPS:
                            xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("pps"));
                            break;

                        case GpxFixType::Unknown:
                        default:
                            break;
                    }

                    // <sat>
                    if (trkpt->satellitesUsedForFixCalculation >= 0)
                        xmlWriter.writeTextElement(QLatin1String("sat"), QString::number(trkpt->satellitesUsedForFixCalculation));

                    // <hdop>
                    if (!qIsNaN(trkpt->horizontalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QLatin1String("hdop"), QString::number(trkpt->horizontalDilutionOfPrecision, 'f', 12));

                    // <vdop>
                    if (!qIsNaN(trkpt->verticalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QLatin1String("vdop"), QString::number(trkpt->verticalDilutionOfPrecision, 'f', 12));

                    // <pdop>
                    if (!qIsNaN(trkpt->positionDilutionOfPrecision))
                        xmlWriter.writeTextElement(QLatin1String("pdop"), QString::number(trkpt->positionDilutionOfPrecision, 'f', 12));

                    // <ageofdgpsdata>
                    if (!qIsNaN(trkpt->ageOfGpsData))
                        xmlWriter.writeTextElement(QLatin1String("ageofdgpsdata"), QString::number(trkpt->ageOfGpsData, 'f', 12));

                    // <dgpsid>
                    if (trkpt->dgpsStationId >= 0)
                        xmlWriter.writeTextElement(QLatin1String("dgpsid"), QString::number(trkpt->dgpsStationId));
                }

                // Write extensions
                if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(trackPoint->extraData.shared_ptr()))
                    writeExtensions(extensions, xmlWriter);

                // </trkpt>
                xmlWriter.writeEndElement();
            }
            
            // Write extensions
            if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(trackSegment->extraData.shared_ptr()))
                writeExtensions(extensions, xmlWriter);

            // </trkseg>
            xmlWriter.writeEndElement();
        }

        // </trk>
        xmlWriter.writeEndElement();
    }
    
    // <rte>'s
    for (const auto& route : constOf(routes))
    {
        // <rte>
        xmlWriter.writeStartElement(QLatin1String("rte"));

        // <name>
        if (!route->name.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("name"), route->name);

        // <desc>
        if (!route->description.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("desc"), route->description);

        // <cmt>
        if (!route->comment.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("cmt"), route->comment);

        // <type>
        if (!route->type.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("type"), route->type);

        // Links
        if (!route->links.isEmpty())
            writeLinks(route->links, xmlWriter);

        if (const auto rte = std::dynamic_pointer_cast<const GpxRte>(route.shared_ptr()))
        {
            // <src>
            if (!rte->source.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("src"), rte->source);

            // <number>
            if (rte->slotNumber >= 0)
                xmlWriter.writeTextElement(QLatin1String("sat"), QString::number(rte->slotNumber));
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(route->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // Write route points
        for (const auto& routePoint : constOf(route->points))
        {
            // <rtept>
            xmlWriter.writeStartElement(QLatin1String("rtept"));
            xmlWriter.writeAttribute(QLatin1String("lat"), QString::number(routePoint->position.latitude, 'f', 12));
            xmlWriter.writeAttribute(QLatin1String("lon"), QString::number(routePoint->position.longitude, 'f', 12));

            // <name>
            if (!routePoint->name.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("name"), routePoint->name);

            // <desc>
            if (!routePoint->description.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("desc"), routePoint->description);

            // <ele>
            if (!qIsNaN(routePoint->elevation))
                xmlWriter.writeTextElement(QLatin1String("ele"), QString::number(routePoint->elevation, 'f', 12));

            // <time>
            if (!routePoint->timestamp.isNull())
                xmlWriter.writeTextElement(QLatin1String("time"), routePoint->timestamp.toString(QLatin1String("YYYY-MM-DDThh:mm:ss")));

            // <cmt>
            if (!routePoint->comment.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("cmt"), routePoint->comment);

            // <type>
            if (!routePoint->type.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("type"), routePoint->type);

            // Links
            if (!routePoint->links.isEmpty())
                writeLinks(routePoint->links, xmlWriter);

            if (const auto rtept = std::dynamic_pointer_cast<const GpxRtePt>(routePoint.shared_ptr()))
            {
                // <magvar>
                if (!qIsNaN(rtept->magneticVariation))
                    xmlWriter.writeTextElement(QLatin1String("magvar"), QString::number(rtept->magneticVariation, 'f', 12));

                // <geoidheight>
                if (!qIsNaN(rtept->geoidHeight))
                    xmlWriter.writeTextElement(QLatin1String("geoidheight"), QString::number(rtept->geoidHeight, 'f', 12));

                // <src>
                if (!rtept->source.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("src"), rtept->source);

                // <sym>
                if (!rtept->symbol.isEmpty())
                    xmlWriter.writeTextElement(QLatin1String("sym"), rtept->symbol);

                // <fix>
                switch (rtept->fixType)
                {
                    case GpxFixType::None:
                        xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("none"));
                        break;

                    case GpxFixType::PositionOnly:
                        xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("2d"));
                        break;

                    case GpxFixType::PositionAndElevation:
                        xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("3d"));
                        break;

                    case GpxFixType::DGPS:
                        xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("dgps"));
                        break;

                    case GpxFixType::PPS:
                        xmlWriter.writeTextElement(QLatin1String("fix"), QLatin1String("pps"));
                        break;

                    case GpxFixType::Unknown:
                    default:
                        break;
                }

                // <sat>
                if (rtept->satellitesUsedForFixCalculation >= 0)
                    xmlWriter.writeTextElement(QLatin1String("sat"), QString::number(rtept->satellitesUsedForFixCalculation));

                // <hdop>
                if (!qIsNaN(rtept->horizontalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QLatin1String("hdop"), QString::number(rtept->horizontalDilutionOfPrecision, 'f', 12));

                // <vdop>
                if (!qIsNaN(rtept->verticalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QLatin1String("vdop"), QString::number(rtept->verticalDilutionOfPrecision, 'f', 12));

                // <pdop>
                if (!qIsNaN(rtept->positionDilutionOfPrecision))
                    xmlWriter.writeTextElement(QLatin1String("pdop"), QString::number(rtept->positionDilutionOfPrecision, 'f', 12));

                // <ageofdgpsdata>
                if (!qIsNaN(rtept->ageOfGpsData))
                    xmlWriter.writeTextElement(QLatin1String("ageofdgpsdata"), QString::number(rtept->ageOfGpsData, 'f', 12));

                // <dgpsid>
                if (rtept->dgpsStationId >= 0)
                    xmlWriter.writeTextElement(QLatin1String("dgpsid"), QString::number(rtept->dgpsStationId));
            }

            // Write extensions
            if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(routePoint->extraData.shared_ptr()))
                writeExtensions(extensions, xmlWriter);

            // </rtept>
            xmlWriter.writeEndElement();
        }
       
        // </rte>
        xmlWriter.writeEndElement();
    }

    // </gpx>
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();

    return true;
}

void OsmAnd::GpxDocument::writeLinks(const QList< Ref<Link> >& links, QXmlStreamWriter& xmlWriter)
{
    for (const auto& link : links)
    {
        // <link>
        xmlWriter.writeStartElement(QLatin1String("link"));
        xmlWriter.writeAttribute(QLatin1String("href"), link->url.toString());

        // <text>
        if (!link->text.isEmpty())
            xmlWriter.writeTextElement(QLatin1String("text"), link->text);

        if (const auto gpxLink = std::dynamic_pointer_cast<const GpxLink>(link.shared_ptr()))
        {
            // <type>
            if (!gpxLink->type.isEmpty())
                xmlWriter.writeTextElement(QLatin1String("type"), gpxLink->type);
        }

        // </link>
        xmlWriter.writeEndElement();
    }
}

void OsmAnd::GpxDocument::writeExtensions(const std::shared_ptr<const GpxExtensions>& extensions, QXmlStreamWriter& xmlWriter)
{
    // <extensions>
    xmlWriter.writeStartElement(QLatin1String("extensions"));
    for (const auto attributeEntry : rangeOf(constOf(extensions->attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    for (const auto& extension : constOf(extensions->extensions))
        writeExtension(extension, xmlWriter);

    // </extensions>
    xmlWriter.writeEndElement();
}

void OsmAnd::GpxDocument::writeExtension(const std::shared_ptr<const GpxExtension>& extension, QXmlStreamWriter& xmlWriter)
{
    // <*>
    xmlWriter.writeStartElement(extension->name);
    for (const auto attributeEntry : rangeOf(constOf(extension->attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    if (!extension->value.isEmpty())
        xmlWriter.writeCharacters(extension->value);

    for (const auto& extension : constOf(extension->subextensions))
        writeExtension(extension, xmlWriter);

    // </*>
    xmlWriter.writeEndElement();
}

bool OsmAnd::GpxDocument::saveTo(QIODevice& ioDevice) const
{
    QXmlStreamWriter xmlWriter(&ioDevice);
    xmlWriter.setAutoFormatting(true);
    return saveTo(xmlWriter);
}

bool OsmAnd::GpxDocument::saveTo(const QString& filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    const bool ok = saveTo(file);
    file.close();

    return ok;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(QXmlStreamReader& xmlReader)
{
    std::shared_ptr<GpxDocument> document;
    std::shared_ptr<GpxMetadata> metadata;
    std::shared_ptr<GpxWpt> wpt;
    std::shared_ptr<GpxTrk> trk;
    std::shared_ptr<GpxRte> rte;
    std::shared_ptr<GpxLink> link;
    std::shared_ptr<GpxRtePt> rtept;
    std::shared_ptr<GpxTrkPt> trkpt;
    std::shared_ptr<GpxTrkSeg> trkseg;
    std::shared_ptr<GpxExtensions> extensions;
    QStack< std::shared_ptr<GpxExtension> > extensionStack;
    
    enum class Token
    {
        gpx,
        metadata,
        wpt,
        trk,
        rte,
        link,
        rtept,
        trkpt,
        trkseg,
        extensions,
    };
    QStack<Token> tokens;

    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (extensions)
            {
                const std::shared_ptr<GpxExtension> extension(new GpxExtension());
                extension->name = tagName.toString();
                for (const auto& attribute : xmlReader.attributes())
                    extension->attributes[attribute.name().toString()] = attribute.value().toString();

                extensionStack.push(extension);
                continue;
            }
            
            if (tagName == QLatin1String("gpx"))
            {
                if (document)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): more than one <gpx> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                document.reset(new GpxDocument());
                document->version = xmlReader.attributes().value(QLatin1String("version")).toString();
                document->creator = xmlReader.attributes().value(QLatin1String("creator")).toString();

                tokens.push(Token::metadata);
            }
            else if (tagName == QLatin1String("metadata"))
            {
                if (document->metadata)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): more than one <metadata> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (metadata)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <metadata> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                metadata.reset(new GpxMetadata());
                tokens.push(Token::metadata);

                //TODO:<author>
                //TODO:<copyright>
                //TODO:<keywords>
                //TODO:<bounds>
            }
            else if (tagName == QLatin1String("wpt"))
            {
                if (wpt)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <wpt>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                bool ok = true;
                const auto latValue = xmlReader.attributes().value(QLatin1String("lat"));
                const double lat = latValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <wpt> 'lat' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(latValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }
                const auto lonValue = xmlReader.attributes().value(QLatin1String("lon"));
                const double lon = lonValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <wpt> 'lon' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(lonValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                wpt.reset(new GpxWpt());
                wpt->position.latitude = lat;
                wpt->position.longitude = lon;

                tokens.push(Token::wpt);
            }
            else if (tagName == QLatin1String("trk"))
            {
                if (trk)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <trk>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trk.reset(new GpxTrk());

                tokens.push(Token::trk);
            }
            else if (tagName == QLatin1String("rte"))
            {
                if (rte)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <rte>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                rte.reset(new GpxRte());

                tokens.push(Token::rte);
            }
            else if (tagName == QLatin1String("name"))
            {
                const auto name = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->name = name;
                        break;
                    case Token::wpt:
                        wpt->name = name;
                        break;
                    case Token::trkpt:
                        trkpt->name = name;
                        break;
                    case Token::trk:
                        trk->name = name;
                        break;
                    case Token::rtept:
                        rtept->name = name;
                        break;
                    case Token::rte:
                        rte->name = name;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <name> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("desc"))
            {
                const auto description = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->description = description;
                        break;
                    case Token::wpt:
                        wpt->description = description;
                        break;
                    case Token::trkpt:
                        trkpt->description = description;
                        break;
                    case Token::trk:
                        trk->description = description;
                        break;
                    case Token::rtept:
                        rtept->description = description;
                        break;
                    case Token::rte:
                        rte->description = description;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <desc> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("ele"))
            {
                bool ok = false;
                const auto elevationValue = xmlReader.readElementText();
                const auto elevation = elevationValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <ele> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(elevationValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->elevation = elevation;
                        break;
                    case Token::trkpt:
                        trkpt->elevation = elevation;
                        break;
                    case Token::rtept:
                        rtept->elevation = elevation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <ele> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("time"))
            {
                const auto timestampValue = xmlReader.readElementText();
                const auto timestamp = QDateTime::fromString(timestampValue, QLatin1String("YYYY-MM-DDThh:mm:ss"));
                if (!timestamp.isValid() || timestamp.isNull())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <time> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(timestampValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->timestamp = timestamp;
                        break;
                    case Token::wpt:
                        wpt->timestamp = timestamp;
                        break;
                    case Token::trkpt:
                        trkpt->timestamp = timestamp;
                        break;
                    case Token::rtept:
                        rtept->timestamp = timestamp;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <time> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("magvar"))
            {
                bool ok = false;
                const auto magneticVariationValue = xmlReader.readElementText();
                const auto magneticVariation = magneticVariationValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <magvar> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(magneticVariationValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->magneticVariation = magneticVariation;
                        break;
                    case Token::trkpt:
                        trkpt->magneticVariation = magneticVariation;
                        break;
                    case Token::rtept:
                        rtept->magneticVariation = magneticVariation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <magvar> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("geoidheight"))
            {
                bool ok = false;
                const auto geoidHeightValue = xmlReader.readElementText();
                const auto geoidHeight = geoidHeightValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <geoidheight> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(geoidHeightValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->geoidHeight = geoidHeight;
                        break;
                    case Token::trkpt:
                        trkpt->geoidHeight = geoidHeight;
                        break;
                    case Token::rtept:
                        rtept->geoidHeight = geoidHeight;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <geoidheight> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("cmt"))
            {
                const auto comment = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->comment = comment;
                        break;
                    case Token::trkpt:
                        trkpt->comment = comment;
                        break;
                    case Token::trk:
                        trk->comment = comment;
                        break;
                    case Token::rtept:
                        rtept->comment = comment;
                        break;
                    case Token::rte:
                        rte->comment = comment;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <cmt> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("src"))
            {
                const auto source = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->source = source;
                        break;
                    case Token::trkpt:
                        trkpt->source = source;
                        break;
                    case Token::trk:
                        trk->source = source;
                        break;
                    case Token::rtept:
                        rtept->source = source;
                        break;
                    case Token::rte:
                        rte->source = source;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <src> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("sym"))
            {
                const auto symbol = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->symbol = symbol;
                        break;
                    case Token::trkpt:
                        trkpt->symbol = symbol;
                        break;
                    case Token::rtept:
                        rtept->symbol = symbol;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <sym> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("type"))
            {
                const auto type = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->type = type;
                        break;
                    case Token::trkpt:
                        trkpt->type = type;
                        break;
                    case Token::trk:
                        trk->type = type;
                        break;
                    case Token::rtept:
                        rtept->type = type;
                        break;
                    case Token::rte:
                        rte->type = type;
                        break;
                    case Token::link:
                        link->type = type;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <type> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("fix"))
            {
                const auto fixValue = xmlReader.readElementText();
                auto fixType = GpxFixType::Unknown;
                if (fixValue == QLatin1String("none"))
                    fixType = GpxFixType::None;
                else if (fixValue == QLatin1String("2d"))
                    fixType = GpxFixType::PositionOnly;
                else if (fixValue == QLatin1String("3d"))
                    fixType = GpxFixType::PositionAndElevation;
                else if (fixValue == QLatin1String("dgps"))
                    fixType = GpxFixType::DGPS;
                else if (fixValue == QLatin1String("pps"))
                    fixType = GpxFixType::PPS;
                else
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <fix> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(fixValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->fixType = fixType;
                        break;
                    case Token::trkpt:
                        trkpt->fixType = fixType;
                        break;
                    case Token::rtept:
                        rtept->fixType = fixType;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <fix> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("sat"))
            {
                bool ok = false;
                const auto satValue = xmlReader.readElementText();
                const auto satellitesUsedForFixCalculation = satValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <sat> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(satValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;
                    case Token::trkpt:
                        trkpt->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;
                    case Token::rtept:
                        rtept->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <sat> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("hdop"))
            {
                bool ok = false;
                const auto hdopValue = xmlReader.readElementText();
                const auto horizontalDilutionOfPrecision = hdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <hdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(hdopValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <hdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("vdop"))
            {
                bool ok = false;
                const auto vdopValue = xmlReader.readElementText();
                const auto verticalDilutionOfPrecision = vdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <vdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(vdopValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <vdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("pdop"))
            {
                bool ok = false;
                const auto pdopValue = xmlReader.readElementText();
                const auto positionDilutionOfPrecision = pdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <pdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(pdopValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <pdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("ageofdgpsdata"))
            {
                bool ok = false;
                const auto ageofdgpsdataValue = xmlReader.readElementText();
                const auto ageOfGpsData = ageofdgpsdataValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <ageofdgpsdata> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(ageofdgpsdataValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->ageOfGpsData = ageOfGpsData;
                        break;
                    case Token::trkpt:
                        trkpt->ageOfGpsData = ageOfGpsData;
                        break;
                    case Token::rtept:
                        rtept->ageOfGpsData = ageOfGpsData;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <ageofdgpsdata> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("dgpsid"))
            {
                bool ok = false;
                const auto dgpsidValue = xmlReader.readElementText();
                const auto dgpsStationId = dgpsidValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <dgpsid> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(dgpsidValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->dgpsStationId = dgpsStationId;
                        break;
                    case Token::trkpt:
                        trkpt->dgpsStationId = dgpsStationId;
                        break;
                    case Token::rtept:
                        rtept->dgpsStationId = dgpsStationId;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <dgpsid> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("link"))
            {
                if (link)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <link>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                const auto hrefValue = xmlReader.attributes().value(QLatin1String("href")).toString();
                const QUrl url(hrefValue);
                if (!url.isValid())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <link> 'href' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(hrefValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                link.reset(new GpxLink());
                link->url = url;

                tokens.push(Token::link);
            }
            else if (tagName == QLatin1String("text"))
            {
                const auto text = xmlReader.readElementText();

                switch (tokens.top())
                {
                    case Token::link:
                        link->text = text;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <text> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("number"))
            {
                bool ok = false;
                const auto numberValue = xmlReader.readElementText();
                const auto slotNumber = numberValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <number> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(numberValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::trk:
                        trk->slotNumber = slotNumber;
                        break;
                    case Token::rte:
                        rte->slotNumber = slotNumber;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <number> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        continue;
                }
            }
            else if (tagName == QLatin1String("trkpt"))
            {
                if (!trkseg)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): <trkpt> not in <trkseg>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (trkpt)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <trkpt>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                bool ok = true;
                const auto latValue = xmlReader.attributes().value(QLatin1String("lat"));
                const double lat = latValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <rtept> 'lat' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(latValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }
                const auto lonValue = xmlReader.attributes().value(QLatin1String("lon"));
                const double lon = lonValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <rtept> 'lon' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(lonValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trkpt.reset(new GpxTrkPt());
                trkpt->position.latitude = lat;
                trkpt->position.longitude = lon;

                tokens.push(Token::trkpt);
            }
            else if (tagName == QLatin1String("trkseg"))
            {
                if (!trk)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): <trkseg> not in <trk>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (trkseg)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <trkseg>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trkseg.reset(new GpxTrkSeg());

                tokens.push(Token::trkseg);
            }
            else if (tagName == QLatin1String("rtept"))
            {
                if (!rte)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): <rtept> not in <rte>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (rtept)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): nested <rtept>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                bool ok = true;
                const auto latValue = xmlReader.attributes().value(QLatin1String("lat"));
                const double lat = latValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <rtept> 'lat' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(latValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }
                const auto lonValue = xmlReader.attributes().value(QLatin1String("lon"));
                const double lon = lonValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%"PRIi64", %"PRIi64"): invalid <rtept> 'lon' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(lonValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                rtept.reset(new GpxRtePt());
                rtept->position.latitude = lat;
                rtept->position.longitude = lon;

                tokens.push(Token::rtept);
            }
            else if (tagName == QLatin1String("extensions"))
            {
                extensions.reset(new GpxExtensions());
                for (const auto& attribute : xmlReader.attributes())
                    extensions->attributes[attribute.name().toString()] = attribute.value().toString();

                tokens.push(Token::extensions);
            }
            else
            {
                LogPrintf(
                    LogSeverityLevel::Warning,
                    "XML warning (%"PRIi64", %"PRIi64"): unknown <%s> tag",
                    xmlReader.lineNumber(),
                    xmlReader.columnNumber(),
                    qPrintableRef(tagName));
                xmlReader.skipCurrentElement();
                continue;
            }
        }
        else if (xmlReader.isEndElement())
        {
            if (extensions && !extensionStack.isEmpty())
            {
                const auto extension = extensionStack.pop();

                if (extensionStack.isEmpty())
                    extensions->extensions.push_back(extension);
                else
                    extensionStack.top()->subextensions.push_back(extension);
                continue;
            }
            
            if (tagName == QLatin1String("gpx"))
            {
                tokens.pop();
            }
            else if (tagName == QLatin1String("metadata"))
            {
                if (document->metadata)
                    continue;
                
                tokens.pop();

                document->metadata = metadata;
                metadata = nullptr;
            }
            else if (tagName == QLatin1String("wpt"))
            {
                tokens.pop();

                document->locationMarks.append(wpt);
                wpt = nullptr;
            }
            else if (tagName == QLatin1String("trk"))
            {
                tokens.pop();

                document->tracks.append(trk);
                trk = nullptr;
            }
            else if (tagName == QLatin1String("rte"))
            {
                document->routes.append(rte);
                rte = nullptr;

                tokens.pop();
            }
            else if (tagName == QLatin1String("name"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("desc"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("ele"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("time"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("magvar"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("geoidheight"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("cmt"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("src"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("sym"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("type"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("fix"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("sat"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("hdop"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("vdop"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("pdop"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("ageofdgpsdata"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("dgpsid"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("link"))
            {
                tokens.pop();

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->links.append(link);
                        link = nullptr;
                        break;
                    case Token::wpt:
                        wpt->links.append(link);
                        link = nullptr;
                        break;
                    case Token::trk:
                        trk->links.append(link);
                        link = nullptr;
                        break;
                    case Token::rte:
                        rte->links.append(link);
                        link = nullptr;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <link> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        link = nullptr;
                        continue;
                }
            }
            else if (tagName == QLatin1String("text"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("number"))
            {
                // Do nothing
            }
            else if (tagName == QLatin1String("trkpt"))
            {
                tokens.pop();

                trkseg->points.append(trkpt);
                trkpt = nullptr;
            }
            else if (tagName == QLatin1String("trkseg"))
            {
                tokens.pop();

                trk->segments.append(trkseg);
                trkseg = nullptr;
            }
            else if (tagName == QLatin1String("rtept"))
            {
                tokens.pop();

                rte->points.append(rtept);
                rtept = nullptr;
            }
            else if (tagName == QLatin1String("extensions"))
            {
                tokens.pop();

                switch (tokens.top())
                {
                    case Token::gpx:
                        document->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::metadata:
                        metadata->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::wpt:
                        wpt->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trk:
                        trk->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trkseg:
                        trkseg->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trkpt:
                        trkpt->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::rte:
                        rte->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::rtept:
                        rtept->extraData = extensions;
                        extensions = nullptr;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%"PRIi64", %"PRIi64"): unexpected <extensions> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        xmlReader.skipCurrentElement();
                        extensions = nullptr;
                        continue;
                }
            }
        }
        else if (xmlReader.isCharacters())
        {
            if (extensions)
            {
                if (!extensionStack.isEmpty())
                    extensionStack.top()->value = xmlReader.text().toString();
                else
                    extensions->value = xmlReader.text().toString();
            }
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML error: %s (%"PRIi64", %"PRIi64")",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return nullptr;
    }

    return document;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(QIODevice& ioDevice)
{
    QXmlStreamReader xmlReader(&ioDevice);
    return loadFrom(xmlReader);
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const auto gpxDocument = loadFrom(file);
    file.close();

    return gpxDocument;
}

OsmAnd::GpxDocument::GpxExtension::GpxExtension()
{
}

OsmAnd::GpxDocument::GpxExtension::~GpxExtension()
{
}

OsmAnd::GpxDocument::GpxExtensions::GpxExtensions()
{
}

OsmAnd::GpxDocument::GpxExtensions::~GpxExtensions()
{
}

OsmAnd::GpxDocument::GpxLink::GpxLink()
{
}

OsmAnd::GpxDocument::GpxLink::~GpxLink()
{
}

OsmAnd::GpxDocument::GpxMetadata::GpxMetadata()
{
}

OsmAnd::GpxDocument::GpxMetadata::~GpxMetadata()
{
}

OsmAnd::GpxDocument::GpxWpt::GpxWpt()
    : magneticVariation(std::numeric_limits<double>::quiet_NaN())
    , geoidHeight(std::numeric_limits<double>::quiet_NaN())
    , fixType(GpxFixType::Unknown)
    , satellitesUsedForFixCalculation(-1)
    , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , positionDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , ageOfGpsData(std::numeric_limits<double>::quiet_NaN())
    , dgpsStationId(-1)
{
}

OsmAnd::GpxDocument::GpxWpt::~GpxWpt()
{
}

OsmAnd::GpxDocument::GpxTrkPt::GpxTrkPt()
    : magneticVariation(std::numeric_limits<double>::quiet_NaN())
    , geoidHeight(std::numeric_limits<double>::quiet_NaN())
    , fixType(GpxFixType::Unknown)
    , satellitesUsedForFixCalculation(-1)
    , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , positionDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , ageOfGpsData(std::numeric_limits<double>::quiet_NaN())
    , dgpsStationId(-1)
{
}

OsmAnd::GpxDocument::GpxTrkPt::~GpxTrkPt()
{
}

OsmAnd::GpxDocument::GpxTrkSeg::GpxTrkSeg()
{
}

OsmAnd::GpxDocument::GpxTrkSeg::~GpxTrkSeg()
{
}

OsmAnd::GpxDocument::GpxTrk::GpxTrk()
    : slotNumber(-1)
{
}

OsmAnd::GpxDocument::GpxTrk::~GpxTrk()
{
}

OsmAnd::GpxDocument::GpxRtePt::GpxRtePt()
    : magneticVariation(std::numeric_limits<double>::quiet_NaN())
    , geoidHeight(std::numeric_limits<double>::quiet_NaN())
    , fixType(GpxFixType::Unknown)
    , satellitesUsedForFixCalculation(-1)
    , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , positionDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , ageOfGpsData(std::numeric_limits<double>::quiet_NaN())
    , dgpsStationId(-1)
{
}

OsmAnd::GpxDocument::GpxRtePt::~GpxRtePt()
{
}

OsmAnd::GpxDocument::GpxRte::GpxRte()
    : slotNumber(-1)
{
}

OsmAnd::GpxDocument::GpxRte::~GpxRte()
{
}

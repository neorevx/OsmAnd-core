#ifndef _OSMAND_CORE_I_UPDATABLE_MAP_SYMBOLS_GROUP_H_
#define _OSMAND_CORE_I_UPDATABLE_MAP_SYMBOLS_GROUP_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IUpdatableMapSymbolsGroup
    {
    private:
    protected:
        IUpdatableMapSymbolsGroup();
    public:
        virtual ~IUpdatableMapSymbolsGroup();

        virtual bool updatesPresent() = 0;
        virtual bool update() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_UPDATABLE_MAP_SYMBOLS_GROUP_H_)

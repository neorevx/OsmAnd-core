#ifndef _OSMAND_CORE_COLLATORSTRINGMATCHER_H
#define _OSMAND_CORE_COLLATORSTRINGMATCHER_H

#include <OsmAndCore.h>

#include <QString>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class CollatorStringMatcher_P;
    
    class OSMAND_CORE_API CollatorStringMatcher
    {
        Q_DISABLE_COPY_AND_MOVE(CollatorStringMatcher);

    private:
        const QString _part;
        const StringMatcherMode _mode;
        
    protected:
        PrivateImplementation<CollatorStringMatcher_P> _p;
        
    public:
        CollatorStringMatcher(const QString& part, const StringMatcherMode mode);
        virtual ~CollatorStringMatcher();
        
        bool matches(const QString& name) const;

        static bool cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode);
        static bool ccontains(const QString& _base, const QString& _part);
        static bool cstartsWith(const QString& _searchInParam, const QString& _theStart,
                         bool checkBeginning, bool checkSpaces, bool equals);
    };
}


#endif //_OSMAND_CORE_COLLATORSTRINGMATCHER_H

/*
 *  Copyright 2001-2007 Internet2
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * AttributeIssuerInEntityGroupFunctor.cpp
 * 
 * A match function that evaluates to true if the attribute issuer is found in metadata and is a member
 * of the given entity group.
 */

#include "internal.h"
#include "exceptions.h"
#include "attribute/filtering/FilteringContext.h"
#include "attribute/filtering/FilterPolicyContext.h"

using namespace opensaml::saml2md;

namespace shibsp {

    static const XMLCh groupID[] = UNICODE_LITERAL_7(g,r,o,u,p,I,D);

    /**
     * A match function that evaluates to true if the attribute issuer is found in metadata and is a member
     * of the given entity group.
     */
    class SHIBSP_DLLLOCAL AttributeIssuerInEntityGroupFunctor : public MatchFunctor
    {
        const XMLCh* m_group;
    public:
        AttributeIssuerInEntityGroupFunctor(const DOMElement* e) {
            m_group = e ? e->getAttributeNS(NULL,groupID) : NULL;
            if (!m_group || !*m_group)
                throw ConfigurationException("AttributeIssuerInEntityGroup MatchFunctor requires non-empty groupID attribute.");
        }

        bool evaluatePolicyRequirement(const FilteringContext& filterContext) const {
            const RoleDescriptor* issuer = filterContext.getAttributeIssuerMetadata();
            if (!issuer)
                return false;
            const EntitiesDescriptor* group = dynamic_cast<const EntitiesDescriptor*>(issuer->getParent()->getParent());
            while (group) {
                if (XMLString::equals(group->getName(), m_group))
                    return true;
                group = dynamic_cast<const EntitiesDescriptor*>(group->getParent());
            }
            return false;
        }

        bool evaluatePermitValue(const FilteringContext& filterContext, const Attribute& attribute, size_t index) const {
            return evaluatePolicyRequirement(filterContext);
        }
    };

    MatchFunctor* SHIBSP_DLLLOCAL AttributeIssuerInEntityGroupFactory(const std::pair<const FilterPolicyContext*,const DOMElement*>& p)
    {
        return new AttributeIssuerInEntityGroupFunctor(p.second);
    }

};

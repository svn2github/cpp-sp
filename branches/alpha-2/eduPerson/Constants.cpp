/*
 * The Shibboleth License, Version 1.
 * Copyright (c) 2002
 * University Corporation for Advanced Internet Development, Inc.
 * All rights reserved
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution, if any, must include
 * the following acknowledgment: "This product includes software developed by
 * the University Corporation for Advanced Internet Development
 * <http://www.ucaid.edu>Internet2 Project. Alternately, this acknowledegement
 * may appear in the software itself, if and wherever such third-party
 * acknowledgments normally appear.
 *
 * Neither the name of Shibboleth nor the names of its contributors, nor
 * Internet2, nor the University Corporation for Advanced Internet Development,
 * Inc., nor UCAID may be used to endorse or promote products derived from this
 * software without specific prior written permission. For written permission,
 * please contact shibboleth@shibboleth.org
 *
 * Products derived from this software may not be called Shibboleth, Internet2,
 * UCAID, or the University Corporation for Advanced Internet Development, nor
 * may Shibboleth appear in their name, without prior written permission of the
 * University Corporation for Advanced Internet Development.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND WITH ALL FAULTS. ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT ARE DISCLAIMED AND THE ENTIRE RISK
 * OF SATISFACTORY QUALITY, PERFORMANCE, ACCURACY, AND EFFORT IS WITH LICENSEE.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER, CONTRIBUTORS OR THE UNIVERSITY
 * CORPORATION FOR ADVANCED INTERNET DEVELOPMENT, INC. BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* Constants.cpp - eduPerson URI constants

   Scott Cantor
   6/21/02

   $History:$
*/

#ifdef WIN32
# define EDUPERSON_EXPORTS __declspec(dllexport)
#endif

#include <eduPerson.h>

const XMLCh eduPerson::XML::EDUPERSON_NS[] = // urn:mace:eduPerson:1.0
{ chLatin_u, chLatin_r, chLatin_n, chColon, chLatin_m, chLatin_a, chLatin_c, chLatin_e, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chColon,
  chDigit_1, chPeriod, chDigit_0, chNull
};

const XMLCh eduPerson::XML::EDUPERSON_SCHEMA_ID[] = // eduPerson.xsd
{ chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chPeriod,
  chLatin_x, chLatin_s, chLatin_d, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_PRINCIPAL_NAME[] = // urn:mace:eduPerson:1.0:eduPersonPrincipalName
{ chLatin_u, chLatin_r, chLatin_n, chColon, chLatin_m, chLatin_a, chLatin_c, chLatin_e, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chColon,
  chDigit_1, chPeriod, chDigit_0, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_P, chLatin_r, chLatin_i, chLatin_n, chLatin_c, chLatin_i, chLatin_p, chLatin_a, chLatin_l,
  chLatin_N, chLatin_a, chLatin_m, chLatin_e, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_AFFILIATION[] = // urn:mace:eduPerson:1.0:eduPersonAffiliation
{ chLatin_u, chLatin_r, chLatin_n, chColon, chLatin_m, chLatin_a, chLatin_c, chLatin_e, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chColon,
  chDigit_1, chPeriod, chDigit_0, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_A, chLatin_f, chLatin_f, chLatin_i, chLatin_l, chLatin_i, chLatin_a, chLatin_t, chLatin_i, chLatin_o, chLatin_n, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_PRIMARY_AFFILIATION[] = // urn:mace:eduPerson:1.0:eduPersonPrimaryAffiliation
{ chLatin_u, chLatin_r, chLatin_n, chColon, chLatin_m, chLatin_a, chLatin_c, chLatin_e, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chColon,
  chDigit_1, chPeriod, chDigit_0, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_P, chLatin_r, chLatin_i, chLatin_m, chLatin_a, chLatin_r, chLatin_y,
  chLatin_A, chLatin_f, chLatin_f, chLatin_i, chLatin_l, chLatin_i, chLatin_a, chLatin_t, chLatin_i, chLatin_o, chLatin_n, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_ENTITLEMENT[] = // urn:mace:eduPerson:1.0:eduPersonEntitlement
{ chLatin_u, chLatin_r, chLatin_n, chColon, chLatin_m, chLatin_a, chLatin_c, chLatin_e, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n, chColon,
  chDigit_1, chPeriod, chDigit_0, chColon,
  chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_E, chLatin_n, chLatin_t, chLatin_i, chLatin_t, chLatin_l, chLatin_e, chLatin_m, chLatin_e, chLatin_n, chLatin_t, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_PRINCIPAL_NAME_TYPE[] = // eduPersonPrincipalNameType
{ chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_P, chLatin_r, chLatin_i, chLatin_n, chLatin_c, chLatin_i, chLatin_p, chLatin_a, chLatin_l,
  chLatin_N, chLatin_a, chLatin_m, chLatin_e, chLatin_T, chLatin_y, chLatin_p, chLatin_e, chNull
};

const XMLCh eduPerson::Constants::EDUPERSON_AFFILIATION_TYPE[] = // eduPersonAffiliationType
{ chLatin_e, chLatin_d, chLatin_u, chLatin_P, chLatin_e, chLatin_r, chLatin_s, chLatin_o, chLatin_n,
  chLatin_A, chLatin_f, chLatin_f, chLatin_i, chLatin_l, chLatin_i, chLatin_a, chLatin_t, chLatin_i, chLatin_o, chLatin_n,
  chLatin_T, chLatin_y, chLatin_p, chLatin_e, chNull
};

/*----------------------------------------------------------------------------
 *
 * File:
 * eas_xmfdata.c
 *
 * Contents and purpose:
 * XMF File Parser
 *
 * This file contains data definitions for the XMF parser.
 *
 * Copyright Sonic Network Inc. 2005

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *----------------------------------------------------------------------------
 * Revision Control:
 *   $Revision: 547 $
 *   $Date: 2007-01-31 16:30:17 -0800 (Wed, 31 Jan 2007) $
 *----------------------------------------------------------------------------
*/

#include "eas_miditypes.h"
#include "eas_xmf.h"
#include "eas_xmfdata.h"

/*----------------------------------------------------------------------------
 *
 * eas_XMFData
 *
 * Static memory allocation for XMF parser
 *----------------------------------------------------------------------------
*/
S_XMF_DATA eas_XMFData;


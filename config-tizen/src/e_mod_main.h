/*

Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved

This file is part of e17-extra-config-modules
Written by Seunggyun Kim <sgyun.kim@smasung.com>

PROPRIETARY/CONFIDENTIAL

This software is the confidential and proprietary information of
SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
disclose such Confidential Information and shall use it only in
accordance with the terms of the license agreement you entered
into with SAMSUNG ELECTRONICS.

SAMSUNG make no representations or warranties about the suitability
of the software, either express or implied, including but not limited
to the implied warranties of merchantability, fitness for a particular
purpose, or non-infringement. SAMSUNG shall not be liable for any
damages suffered by licensee as a result of using, modifying or
distributing this software or its derivatives.

*/

#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

EAPI extern E_Module_Api e_modapi;

EAPI void* e_modapi_init (E_Module* m);
EAPI int e_modapi_shutdown (E_Module* m);
EAPI int e_modapi_save (E_Module* m);


#endif//__E_MOD_MAIN_H__


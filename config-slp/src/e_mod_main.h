/*
 * e17-extra-config-modules
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

EAPI extern E_Module_Api e_modapi;

EAPI void* e_modapi_init (E_Module* m);
EAPI int e_modapi_shutdown (E_Module* m);
EAPI int e_modapi_save (E_Module* m);


#endif//__E_MOD_MAIN_H__

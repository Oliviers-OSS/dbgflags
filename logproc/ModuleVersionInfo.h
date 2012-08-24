#ifndef _MODULE_VERSION_INFO_H_
#define _MODULE_VERSION_INFO_H_

#ifndef TO_STRING
#define STRING(x) #x
#define TO_STRING(x) STRING(x)
#endif

#define MODULE_NAME(name)						   \
static const char __module_name[] __attribute__((section(".modinfo"))) = 	   \
"Name=" #name

#define MODULE_AUTHOR(name)						   \
static const char __module_author[] __attribute__((section(".modinfo"))) = 	   \
"Author=" #name

#define MODULE_VERSION(modinfo)						   \
static const char __module_modinfo[] __attribute__((section(".modinfo"))) = 	   \
"Version=" #modinfo

#define MODULE_FILE_VERSION(modinfo)						   \
static const char __module_file_modinfo[] __attribute__((section(".modinfo"))) = 	   \
"File modinfo=" #modinfo

#define MODULE_CLEARCASE_LABEL(label)						   \
static const char __module_cc_label[] __attribute__((section(".modinfo"))) = 	   \
"Clearcase Label=" #label

#define MODULE_DESCRIPTION(description)						   \
static const char __module_description[] __attribute__((section(".modinfo"))) = 	   \
"Description=" #description

#define MODULE_MANUFACTURER(manufacturer)						   \
static const char __module_description[] __attribute__((section(".modinfo"))) = 	   \
"Manufacturer=" #manufacturer

#define MODULE_COPYRIGHT(copyright)						   \
static const char __module_copyright[] __attribute__((section(".modinfo"))) = 	   \
"Copyright=" #copyright

#define MODULE_LANGUAGE(language)						   \
static const char __module_language[] __attribute__((section(".modinfo"))) = 	   \
"Language=" #language

#define MODULE_NAME_AUTOTOOLS						   \
static const char __module_name[] __attribute__((section(".modinfo"))) = 	   \
"Name=" PACKAGE_NAME

#define MODULE_VERSION_AUTOTOOLS						   \
static const char __module_modinfo[] __attribute__((section(".modinfo"))) = 	   \
"Version=" PACKAGE_VERSION

#define MODULE_AUTHOR_AUTOTOOLS						   \
static const char __module_author[] __attribute__((section(".modinfo"))) = 	   \
"Author=" PACKAGE_BUGREPORT

#define PACKAGE_NAME_AUTOTOOLS						   \
static const char __package_name[] __attribute__((section(".modinfo"))) = 	   \
"Package=" PACKAGE_NAME

#define MODULE_LOGGER                                                  \
static const char __module_logger[] __attribute__((section(".modinfo"))) = 	   \
"Logger=" TO_STRING(LOGGER)

#endif /* _MODULE_VERSION_INFO_H_ */

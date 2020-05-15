TEMPLATE = subdirs

SUBDIRS = \
    apps \
    libs \
    tests \

apps.depends = libs
tests.depends = libs

TRANSLATIONS = ./i18n/librepcb.ts

# Print information about current linking configuration
isEmpty(UNBUNDLE) {
    message("Link statically")
} else {
    message("Link dynamically")
}
contains(UNBUNDLE, quazip) {
    message("Using system quazip")
} else {
    message("Using vendored quazip")
}

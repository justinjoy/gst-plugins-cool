dnl configure-time options shared among gstreamer modules

dnl AG_GST_CHECK_PLUGIN_WITH_DEFAULT_FEATURE(PLUGIN-NAME, DEFAULT-FEATURE)
dnl example
dnl AG_GST_CHECK_PLUGIN_WITH_DEFAULT_FEATURE(decproxy, yes)
dnl AG_GST_CHECK_PLUGIN_WITH_DEFAULT_FEATURE(fakedec, no)
dnl
dnl This macro adds the plug-in <PLUGIN-NAME> to GST_PLUGINS_ALL. Then it
dnl checks if WITH_PLUGINS is empty or the plugin is present in WITH_PLUGINS,
dnl and if so adds it to GST_PLUGINS_SELECTED. Then it checks if the plugin
dnl is present in WITHOUT_PLUGINS (ie. was disabled specifically) and if so
dnl removes it from GST_PLUGINS_SELECTED.
dnl
dnl The macro will call AM_CONDITIONAL(USE_PLUGIN_<PLUGIN-NAME>, ...) to allow
dnl control of what is built in Makefile.ams.
AC_DEFUN([AG_GST_CHECK_PLUGIN_WITH_DEFAULT_FEATURE],
[
  GST_PLUGINS_ALL="$GST_PLUGINS_ALL [$1]"

  define([pname_def],translit([$1], -a-z, _a-z))

  if test x$[$2] = xyes; then
    default_feature=disable
  else
    default_feature=enable
  fi

  AC_ARG_ENABLE([$1],
    AC_HELP_STRING([--$default_feature-[$1]], [[enable|disable] $1 plugin [[default=$2]]]),
    [
      case "${enableval}" in
        yes) [gst_use_]pname_def=yes ;;
        no) [gst_use_]pname_def=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-$1]) ;;
       esac
    ],
    [[gst_use_]pname_def=[$2]]) dnl Default value

  if test x$[gst_use_]pname_def = xno; then
    AC_MSG_NOTICE(disabling dependency-less plugin $1)
    WITHOUT_PLUGINS="$WITHOUT_PLUGINS [$1]"
  fi
  undefine([pname_def])

  dnl First check inclusion
  if [[ -z "$WITH_PLUGINS" ]] || echo " [$WITH_PLUGINS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    GST_PLUGINS_SELECTED="$GST_PLUGINS_SELECTED [$1]"
  fi
  dnl Then check exclusion
  if echo " [$WITHOUT_PLUGINS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    GST_PLUGINS_SELECTED=`echo " $GST_PLUGINS_SELECTED " | $SED -e 's/ [$1] / /'`
  fi
  dnl Finally check if the plugin is ported or not
  if echo " [$GST_PLUGINS_NONPORTED] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    GST_PLUGINS_SELECTED=`echo " $GST_PLUGINS_SELECTED " | $SED -e 's/ [$1] / /'`
  fi
  AM_CONDITIONAL([USE_PLUGIN_]translit([$1], a-z, A-Z), echo " $GST_PLUGINS_SELECTED " | grep -i " [$1] " > /dev/null)
])

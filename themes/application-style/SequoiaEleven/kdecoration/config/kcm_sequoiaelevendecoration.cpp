#include "sequoiaelevenconfigwidget.h"
#include <KPluginFactory>

#ifdef SEQUOIA_GLASS_TRIAL
K_PLUGIN_CLASS_WITH_JSON(SequoiaEleven::ConfigWidget, "kcm_sequoiaelevenglassrim.json")
#else
K_PLUGIN_CLASS_WITH_JSON(SequoiaEleven::ConfigWidget, "kcm_sequoiaelevendecoration.json")
#endif

#include "kcm_sequoiaelevendecoration.moc"

#!/usr/bin/python
# -*- coding: UTF-8 -*-

path_cfgCats="../../../nom_WS/cats.cfg"
path_sysList="../../xmlAnaWSBuilder/run/configAllCats/vbf_cp_m00/channel/category_OO_TT_b1.xml"

path_yySysList="yy_sysList.txt"

combPois=['mu', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']
yyPois=['mu', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'dummy', 'dummy']
ttPois=['dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']

combOutRootFile="workspace/combined_{dtilde}.root"

yyInRootFile="workspace/hgg_param.root"
yyInRootFile="inWS/gamgam/vbf_cp_{dtilde}.root"
ttInRootFile="inWS/tautau/{dtilde}/VBFCPCOMBVBFMVA_FSPLIT_combined_VBFCPCOMBVBFMVA_FSPLIT_model.root"

outXml="xml/comb_{dtilde}.xml"

chlistName = {
'diphoton':'hgg',
'ditau':'htt',
}

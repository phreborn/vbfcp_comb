#!/usr/bin/python
# -*- coding: UTF-8 -*-

path_cfgCats="../../../nom_WS/cats.cfg"
path_sysList="../../xmlAnaWSBuilder/run/configAllCats/vbf_cp_m00/channel/category_OO_LT_b3.xml"

path_yySysList="yy_sysList.txt"
path_ttSysList="tautau_npList.txt"

combPois=['mu', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']
yyPois=['mu', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'dummy', 'dummy']
ttPois=['dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']

combOutRootFile="workspace/combined_{dtilde}.root"

yyInRootFile="inWS/gamgam/Expected/vbf_cp_{dtilde}.root"
ttInRootFile="inWS/tautau/{dtilde}/125.root"
yydata='asimovData_SB_SM'
ttdata='asimovData'

yyInRootFile="inWS/gamgam/Observed/vbf_cp_{dtilde}.root"
ttInRootFile="inWS/tautau/{dtilde}/125.root"
yydata='combData'
ttdata='obsData'

outXml="xml/comb_{dtilde}.xml"

chlistName = {
'diphoton':'hgg',
'ditau':'htt',
}

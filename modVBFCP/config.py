#!/usr/bin/python
# -*- coding: UTF-8 -*-

path_cfgCats="../../../nom_WS/cats.cfg"

path_yySysList="yy_nuisList.txt"
path_ttSysList="tautau_nuisList.txt"

combPois=['mu', 'mu_BR_yy', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'mu_spur', 'mu_spur_SM', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']
yyPois=['mu', 'mu_BR_yy', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'mu_spur', 'mu_spur_SM', 'dummy', 'dummy']
ttPois=['dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected']

combOutRootFile="workspaces/combined_{dtilde}.root"

yyInRootFile="inWS/gamgam/Observed/vbf_cp_{dtilde}_mod.root"
yydata='combData'
ttInRootFile="inWS/tautau/{dtilde}/125_lumi_split.root"
ttdata='obsData'

outXml="combXml/comb_{dtilde}.xml"

chlistName = {
'diphoton':'hgg',
'ditau':'htt',
}

ttMaps={'ATLAS_Forward_JVT':'ATLAS_fJVT',
'ATLAS_JVT':'ATLAS_JVT',
#'Theo_sig_alphaS':'TheorySig_QCDalphaS',
'Theo_sig_pdf_1':'TheorySig_PDF_1',
'Theo_sig_pdf_4':'TheorySig_PDF_4',
'Theo_sig_pdf_16':'TheorySig_PDF_16',
'ATLAS_LUMI_CORR':'ATLAS_LUMI_CORR',
'ATLAS_LUMI_UNCORR_1516':'ATLAS_LUMI_UNCORR_1516',
}

yyMaps={'ATLAS_JET_JvtEfficiency':'ATLAS_JVT',
'ATLAS_JET_fJvtEfficiency':'ATLAS_fJVT',
#'QCDalphaS':'TheorySig_QCDalphaS',
'ATLAS_pdf_1':'TheorySig_PDF_1',
'ATLAS_pdf_2':'TheorySig_PDF_2',
'ATLAS_pdf_3':'TheorySig_PDF_3',
'ATLAS_pdf_4':'TheorySig_PDF_4',
'ATLAS_pdf_5':'TheorySig_PDF_5',
'ATLAS_pdf_6':'TheorySig_PDF_6',
'ATLAS_pdf_7':'TheorySig_PDF_7',
'ATLAS_pdf_8':'TheorySig_PDF_8',
'ATLAS_pdf_9':'TheorySig_PDF_9',
'ATLAS_pdf_10':'TheorySig_PDF_10',
'ATLAS_pdf_11':'TheorySig_PDF_11',
'ATLAS_pdf_12':'TheorySig_PDF_12',
'ATLAS_pdf_13':'TheorySig_PDF_13',
'ATLAS_pdf_14':'TheorySig_PDF_14',
'ATLAS_pdf_15':'TheorySig_PDF_15',
'ATLAS_pdf_16':'TheorySig_PDF_16',
'ATLAS_pdf_17':'TheorySig_PDF_17',
'ATLAS_pdf_18':'TheorySig_PDF_18',
'ATLAS_pdf_19':'TheorySig_PDF_19',
'ATLAS_pdf_20':'TheorySig_PDF_20',
'ATLAS_pdf_21':'TheorySig_PDF_21',
'ATLAS_pdf_22':'TheorySig_PDF_22',
'ATLAS_pdf_23':'TheorySig_PDF_23',
'ATLAS_pdf_24':'TheorySig_PDF_24',
'ATLAS_pdf_25':'TheorySig_PDF_25',
'ATLAS_pdf_26':'TheorySig_PDF_26',
'ATLAS_pdf_27':'TheorySig_PDF_27',
'ATLAS_pdf_28':'TheorySig_PDF_28',
'ATLAS_pdf_29':'TheorySig_PDF_29',
'ATLAS_pdf_30':'TheorySig_PDF_30',
'ATLAS_LUMI_CORR':'ATLAS_LUMI_CORR',
'ATLAS_LUMI_UNCORR_1516':'ATLAS_LUMI_UNCORR_1516',
'ATLAS_LUMI_UNCORR_1718':'ATLAS_LUMI_UNCORR_1718',
'BR_HiggsDecayWidthTHU_hbb':'TheorySig_BR_HiggsDecayWidthTHU_hbb',
'BR_HiggsDecayWidthTHU_hgg':'TheorySig_BR_HiggsDecayWidthTHU_hgg',
'BR_HiggsDecayWidthTHU_hVV':'TheorySig_BR_HiggsDecayWidthTHU_hVV',
'BR_HiggsDecayWidthTHU_hyy':'TheorySig_BR_HiggsDecayWidthTHU_hyy',
'BR_param_mB':'TheorySig_BR_param_mB',
'BR_param_mC':'TheorySig_BR_param_mC',
}

elseConstr=['ATLAS_LUMI', 'BR_HiggsDecay', 'BR_param']

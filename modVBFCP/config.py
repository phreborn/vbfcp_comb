#!/usr/bin/python
# -*- coding: UTF-8 -*-

path_cfgCats="../../../nom_WS/cats.cfg"

path_yySysList="yy_nuisList.txt"
path_ttSysList="tautau_nuisList.txt"

combPois=['mu', 'mu_BR_yy', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'mu_spur', 'mu_spur_SM', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected', 'mu_Htau', 'mu_BR_tautau_mod', 'mu_BR_WW_mod', 'mu_VBF', 'mu_otherH']
yyPois=['mu', 'mu_BR_yy', 'mu_VBF_RW', 'mu_VBF_SM', 'mu_ggH', 'mu_ggH_SM', 'mu_spur', 'mu_spur_SM', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy']
ttPois=['dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'dummy', 'ATLAS_epsilon', 'ATLAS_epsilon_rejected', 'mu_Htau', 'mu_BR_tautau_mod', 'mu_BR_WW_mod', 'mu_VBF', 'mu_otherH']

combOutRootFile="workspaces/combined_{dtilde}.root"

yyInRootFile="inWS/gamgam/Observed/vbf_cp_{dtilde}_mod.root"
yydata='combData'
ttInRootFile="inWS/tautau/{dtilde}/125_lumi_split_mod.root"
ttdata='obsData'

outXml="combXml/comb_{dtilde}.xml"
outXml="combXml/example.xml"

chlistName = {
'diphoton':'hgg',
'ditau':'htt',
}

ttMaps={
'alpha_Theo_sig_pdf_1':'TheorySig_PDF_1',
'alpha_Theo_sig_pdf_4':'TheorySig_PDF_4',
'alpha_Theo_sig_pdf_16':'TheorySig_PDF_16',
'ATLAS_LUMI_CORR':'ATLAS_LUMI_CORR',
'ATLAS_LUMI_UNCORR_1516':'ATLAS_LUMI_UNCORR_1516',
'QCDalphaS':'TheorySig_QCDalphaS',
'BR_HiggsDecayWidthTHU_hbb':'TheorySig_BR_HiggsDecayWidthTHU_hbb',
'BR_HiggsDecayWidthTHU_hgg':'TheorySig_BR_HiggsDecayWidthTHU_hgg',
'BR_HiggsDecayWidthTHU_hVV':'TheorySig_BR_HiggsDecayWidthTHU_hVV',
'BR_HiggsDecayWidthTHU_htautau':'TheorySig_BR_HiggsDecayWidthTHU_htautau',
'BR_param_mB':'TheorySig_BR_param_mB',
'BR_param_mC':'TheorySig_BR_param_mC',
'alpha_ATLAS_EG_RESOLUTION_ALL':'ATLAS_EG_RESOLUTION_ALL',
'alpha_ATLAS_EG_SCALE_ALLCORR':'ATLAS_EG_SCALE_ALL',
'alpha_Theo_ggH_shower':'TheorySig_shower_ggF',
'alpha_Theo_VBFH_shower':'TheorySig_shower_VBF',
'alpha_Theo_ggH_sig_qcd_1':'TheorySig_QCDscale_ggF_res',
'alpha_Theo_ggH_sig_qcd_2':'TheorySig_QCDscale_ggF_mig01',
'alpha_Theo_ggH_sig_qcd_3':'TheorySig_QCDscale_ggF_mig12',
'alpha_Theo_ggH_sig_qcd_6':'TheorySig_QCDscale_ggF_pTH60',
'alpha_Theo_ggH_sig_qcd_7':'TheorySig_QCDscale_ggF_pTH120',
}

yyMaps={
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
'QCDalphaS':'TheorySig_QCDalphaS',
'BR_HiggsDecayWidthTHU_hbb':'TheorySig_BR_HiggsDecayWidthTHU_hbb',
'BR_HiggsDecayWidthTHU_hgg':'TheorySig_BR_HiggsDecayWidthTHU_hgg',
'BR_HiggsDecayWidthTHU_hVV':'TheorySig_BR_HiggsDecayWidthTHU_hVV',
'BR_HiggsDecayWidthTHU_hyy':'TheorySig_BR_HiggsDecayWidthTHU_hyy',
'BR_param_mB':'TheorySig_BR_param_mB',
'BR_param_mC':'TheorySig_BR_param_mC',
'ATLAS_EG_RESOLUTION_ALL':'ATLAS_EG_RESOLUTION_ALL',
'ATLAS_EG_SCALE_ALL':'ATLAS_EG_SCALE_ALL',
'ATLAS_shower_ggF':'TheorySig_shower_ggF',
'ATLAS_shower_VBF':'TheorySig_shower_VBF',
'ATLAS_qcd2_ggF':'TheorySig_QCDscale_ggF_res',
'ATLAS_qcd3_ggF':'TheorySig_QCDscale_ggF_mig01',
'ATLAS_qcd4_ggF':'TheorySig_QCDscale_ggF_mig12',
'ATLAS_qcd6_ggF':'TheorySig_QCDscale_ggF_pTH60',
'ATLAS_qcd7_ggF':'TheorySig_QCDscale_ggF_pTH120',
}

elseConstr=['ATLAS_LUMI', 'BR_HiggsDecay', 'BR_param', 'QCDalphaS']

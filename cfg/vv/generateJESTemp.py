#!/usr/bin/python
import sys, os, time, commands, getopt, copy, random, math
from glob import glob
if __name__=="__main__" :
    if len(sys.argv)<4:
        print "Usage: python "+sys.argv[0]+" <input file name> <output file name> <mode>"
        sys.exit()
    else:
        print sys.argv
        
    inputFileName=sys.argv[1]
    outputFileName=sys.argv[2]
    mode=sys.argv[3]
    
    inputFile=open(inputFileName,"r")
    outputFile=open(outputFileName,"w")

    originalNP=["ATLAS_JET_GroupedNP_2", "ATLAS_JET_WZ_Run1_pT", "ATLAS_JET_WZ_Run1_mass", "ATLAS_JET_WZ_Run1_D2", "ATLAS_FATJET_JER", "ATLAS_FATJET_JMR"]
    nominalNP=["ATLAS_Jet_Scale1", "ATLAS_Jet_Scale1", "ATLAS_Jet_Scale1", "ATLAS_Jet_Scale1", "ATLAS_LargeR_Res1", "ATLAS_LargeR_Res2"]
    jesNP=["ATLAS_Jet_Scale1", "ATLAS_Jet_Scale1", "ATLAS_LargeR_Scale1", "ATLAS_LargeR_Scale1", "ATLAS_LargeR_Res1", "ATLAS_LargeR_Res1"]
    largeRNP=["ATLAS_Jet_Scale1", "ATLAS_LargeR_Scale1", "ATLAS_LargeR_Scale1", "ATLAS_LargeR_Scale1", "ATLAS_LargeR_Res1", "ATLAS_LargeR_Res1"]

    modeArr=[]
    if mode=="nominal": modeArr=nominalNP
    elif mode=="jes": modeArr=jesNP
    elif mode=="largeR": modeArr=largeRNP
    else:
        print "Unknown mode "+mode
        os.abort()
        
    for line in inputFile:
        for idx in range(0, len(originalNP)):
            if originalNP[idx] in line:
                line=line.replace(originalNP[idx], modeArr[idx])
        outputFile.write(line)

    inputFile.close()
    outputFile.close()

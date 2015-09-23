/*
 * =====================================================================================
 *
 *       Filename:  combiner.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  haoshuang.ji (), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */

#include "TFile.h"
#include "TString.h"
#include "TList.h"
#include "TDOMParser.h"
#include "TXMLDocument.h"
#include "TXMLNode.h"
#include "TXMLAttr.h"
#include "TCanvas.h"

#include "rooCommon.h"

#include "asimovUtils.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <istream>
#include <memory>
#include <vector>
#include <map>
#include <string>
struct POI {
  std::string name;
  double min;
  double max;
};

struct Channel {
  std::string name_;   /* name of itself */
  double mass_;
  std::string fileName_;
  std::string wsName_;
  std::string mcName_;
  std::string poiName_;
  std::string dataName_;
  std::map<std::string, std::string> renameMap_;
  std::map<std::string, std::string> pdfReMap_;

  void operator=(const Channel& ch)
  {
    name_ = ch.name_;
    mass_ = ch.mass_;
    fileName_ = ch.fileName_;
    wsName_ = ch.wsName_;
    mcName_ = ch.mcName_;
    poiName_ = ch.poiName_;
    dataName_ = ch.dataName_;
    renameMap_ = ch.renameMap_;
    pdfReMap_ = ch.pdfReMap_;
  }

    void Print()
  {
    std::cout << "Channel Name: " << name_ << std::endl;
    std::cout << "\tFile Name: " << fileName_ << std::endl;
    std::cout << "\tWorkspace Name: " << wsName_ << std::endl;
    std::cout << "\tPOI Name: " << poiName_ << std::endl;
    std::cout << "\tData Name: " << dataName_ << std::endl;
    typedef std::map<std::string, std::string>::iterator it_type;
    for(it_type iterator = renameMap_.begin(); iterator != renameMap_.end(); iterator++) {
      std::cout << "\t\tOldNuis Name: " << iterator->first << " ---> NewNuis Name: " << iterator->second << std::endl;
    }
  }
};

class combiner {
 public:
  combiner();
  ~combiner();

  enum keepTmp{ReadTmp, WriteTmp, nRnWTmp};
  void initWorkspace(std::string fileName, enum keepTmp what);
  void makeSimCategory();
  void regularizeWorkspace();
  void makeSnapshots0(
		      std::string minimizerType,
		      std::string inputFile,
		      std::string snapshotHintFile="",
		      double tolerance=0.001,
		      bool simple=false,
		      int fitFlag=0,
		      bool multiplePoi=false
		      );
  void static makeSnapshots(
			    std::string minimizerType,
			    std::string inputFile,
			    std::string snapshotHintFile="",
			    double tolerance=0.001,
			    bool simple = false,
			    int fitFlag=0,
			    bool multiplePoi=false
			    );


  void doFit(double mu=-1);
  void editBR(RooAbsPdf* pdf, RooWorkspace* w, std::map<std::string, std::string>& renameMap);
  void editPDF(RooAbsPdf* pdf, RooWorkspace* w, std::map<std::string, std::string>& renameMap);


  void readConfigXml( std::string configFileName );
  void readChannel( TXMLNode* rootNode );
  std::string getAttributeValue(
				TXMLNode* rootNode,
				std::string attributeKey = "Name"
				);

  void findArgSetIn(RooWorkspace* w, RooArgSet* set);
  void printSummary()
  {
    int num = (int)m_summary.size();
    for ( int i= 0; i < num; i++ ) {
      m_summary[i].Print();
    }
  }

  void getPOIs(std::string& poisInput,
               std::vector<POI>& pois
	       )
  {
    // mu[0-5], mu_VH[0-15]
    TString poiStr = poisInput.c_str();
    TObjArray* strArray = poiStr.Tokenize(",");
    int nPars = strArray->GetEntries();

    POI poi;

    Ssiz_t index1, index2, index3; /* for [, ] */
    TString subStr;
    TString range;
    for ( int i= 0; i < nPars; i++ ) {
      subStr = ((TObjString*)strArray->At(i))->GetString();
      subStr = subStr.ReplaceAll(" ", "");
      index1 = subStr.First("[");
      if ( index1==-1 ) {
        poi.name = subStr.Data();
        poi.max = poi.min = -999;
      } else {
        index2 = subStr.First("]");
        poi.name = TString(TString(subStr(0, index1))).Data();

        range = TString(TString(subStr(index1+1, index2-index1))).Data();
        index3 = range.First("~");
        poi.min = TString(TString(range(0, index3))).Atof();
        poi.max = TString(TString(range(index3+1, range.Sizeof()-index3))).Atof();
      }

      pois.push_back(poi);
    }
  }

  void deCompose(std::string& gausStr, std::string& pdf,
                 std::string& obs, std::string& mean, double& sigma)
  {
    /* lumiConstraint(Lumi, nominalLumi, 0.37), the default value of sigma(third parameter is 1, can be neglected) */
    TString oldGaus = gausStr.c_str();

    TObjArray* strArray = oldGaus.Tokenize(",");
    int nPars = strArray->GetEntries();

    Ssiz_t index;
    TString tmpStr;
    if ( nPars==2 ) {
      /* first---> lumiConstraint(Lumi */
      tmpStr = ((TObjString*)strArray->At(0))->GetString();
      index = tmpStr.First("(");
      pdf = TString(TString(tmpStr(0, index)).ReplaceAll(" ", "")).Data();
      obs = TString(TString(tmpStr(index+1, tmpStr.Sizeof()-index)).ReplaceAll(" ", "")).Data();

      /* second---> nominalLumi) */
      tmpStr = ((TObjString*)strArray->At(1))->GetString();
      mean = tmpStr.ReplaceAll(" ", "").ReplaceAll(")", "");

      sigma = 1.;
    } else if ( nPars==3 ) {
      /* first---> lumiConstraint(Lumi */
      tmpStr = ((TObjString*)strArray->At(0))->GetString();
      index = tmpStr.First("(");
      pdf = TString(TString(tmpStr(0, index)).ReplaceAll(" ", "")).Data();
      obs = TString(TString(tmpStr(index+1, tmpStr.Sizeof()-index)).ReplaceAll(" ", "")).Data();

      /* second---> nominalLumi */
      tmpStr = ((TObjString*)strArray->At(1))->GetString();
      mean = tmpStr.ReplaceAll(" ", "");

      /* third---> 0.37) */
      tmpStr = ((TObjString*)strArray->At(2))->GetString();
      sigma = tmpStr.ReplaceAll(" ", "").ReplaceAll(")", "").Atof();
    } else {
      std::cout << "The input is wrong! " << std::endl;
      assert ( false );
    }
  }

  static void linkMap(
		      std::map<std::string, std::string>& attMap,
		      std::string& keyStr,
		      std::string& valueStr,
		      std::string linker = ","
		      )
  {
    int mapSize = (int)attMap.size();
    int index = 0;
    typedef std::map<std::string, std::string>::iterator it_type;
    for(it_type iterator = attMap.begin(); iterator != attMap.end(); iterator++) {
      keyStr += iterator->first;
      valueStr += iterator->second;
      if ( index<(mapSize-1) ) {
        keyStr += linker;
        valueStr += linker;
      }
      index += 1;
    }
  }

  static void getBasePdf(RooProdPdf* pdf, RooArgSet& set);

  void write(std::string outputFile="");

  void editBR(bool edit)
  {
    m_editBR = edit;
  }

  void editPDF(bool edit)
  {
    m_editPDF = edit;
  }


  void editRFV(bool edit)
  {
    m_editRFV = edit;
  }


  template<class T>
    void tokenizeV(const std::string &s,
		   std::vector<T> &o)
    {
      typedef boost::tokenizer<boost::char_separator<char> >  tok_t;

      boost::char_separator<char> sep(" \t");
      tok_t tok(s, sep);
      for(tok_t::iterator j (tok.begin());
	  j != tok.end();
	  ++j)
	{
	  std::string f(*j);
	  boost::trim(f);
	  o.push_back(boost::lexical_cast<T>(f));
	}
    }

  /* read text file with title */
  void readTxt(std::string filename,
	       std::vector<std::string>& names,
	       std::map<double, std::vector<double> >& contentMap
	       )
  {
    /* match line containing numbers */
    static const boost::regex e("\\d{2}.*");

    boost::iostreams::stream<boost::iostreams::file_source> file(filename.c_str());
    std::string line;
    while (std::getline(file, line)) {
      bool isNumber =  boost::regex_match(line, e);

      /* replace % with E-2 */
      boost::xpressive::sregex re  =  boost::xpressive::as_xpr("%");
      std::string format("E-2");
      line  =  regex_replace( line, re, format );

      if ( isNumber ) {
	std::vector<double> results;
	tokenizeV(line, results);
	contentMap[results[0]] = results;
      } else {
	tokenizeV(line, names);
      }
    }
  }

  void makeBOnly(bool flag) { m_mkBonly = flag; }


  TString editRFVString( TString& oldString )
  {
    TString newFormExprBegin = "";
    TString newFormExprEnd = "";
    TString templateStr = oldString;
    std::map<std::string, std::string> reNameMap;
    /* want to replace the long ones first, so that there is no mis-replacement */
    std::vector<std::string> allOldNames;
    std::vector<int> indice;
    std::vector<int> nameLength;

    TObjArray* strArray = templateStr.Tokenize(",");
    int num = strArray->GetEntries() - 1;

    newFormExprBegin = ((TObjString*)strArray->At(0))->GetString();

    for ( int i= 0; i < num; i++ ) {
      // TString oldName = _actualVars.at(i)->GetName();
      TString oldName = ((TObjString*)strArray->At(i+1))->GetString();
      oldName = oldName.ReplaceAll(")", "");
      oldName = oldName.ReplaceAll(" ", "");
      newFormExprEnd += ",";
      newFormExprEnd += oldName;
      TString newName = TString::Format("@%d", i);
      indice.push_back(i);
      nameLength.push_back(oldName.Length());
      allOldNames.push_back(oldName.Data());
      reNameMap[oldName.Data()] = newName.Data();
    }
    newFormExprEnd += ")";

    TMath::Sort(num, &nameLength[0], &indice[0], true);
    for ( int i= 0; i < num; i++ ) {
      int index = indice[i];
      TString oldName = allOldNames[index];
      TString newName = reNameMap[oldName.Data()].c_str();
      newFormExprBegin = newFormExprBegin.ReplaceAll(oldName, newName);
    }
    // std::cout << "\tInput : " << oldString << std::endl;
    std::cout << "\tOutput: " << newFormExprBegin+newFormExprEnd << std::endl;
    return newFormExprBegin+newFormExprEnd;
  }
  
 private:
  std::vector<Channel> m_summary;
  std::vector<POI> m_pois;
  Channel m_outSummary;

  std::string m_wsName;
  std::string m_dataName;
  std::string m_pdfName;
  std::string m_poiName;
  std::string m_obsName;
  std::string m_nuisName;
  std::string m_gObsName;
  std::string m_catName;
  std::string m_outputFileName;

  double m_mass;
  bool m_editBR;
  bool m_editPDF;
  bool m_editRFV;
  bool m_mkBonly;

  RooArgSet m_obs;
  RooArgSet m_obsAndWgt;
  RooWorkspace* m_comb;
  RooArgSet* m_nuis;
  RooArgSet* m_gObs;
  RooStats::ModelConfig* m_mc;
  RooWorkspace* m_tmpComb; /* temporary */
  RooArgSet* m_tmpNuis;
  RooArgSet* m_tmpGobs;
  RooCategory* m_cat;
  RooSimultaneous* m_pdf;
  RooDataSet* m_data;
  TFile* m_outputFile;

  int m_numChannel;
  std::map<std::string, RooAbsPdf*> m_pdfMap;
  std::map<std::string, RooDataSet*> m_dataMap;
  TList m_keep;

};

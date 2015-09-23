/*
 * =====================================================================================
 *
 *       Filename:  Orgnizer.h
 *
 *    Description:  Orgnize the workspace
 *
 *        Version:  1.0
 *        Created:  07/19/12 17:32:58
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (hji), haoshuang.ji@cern.ch
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef Manager_Orgnizer
#define Manager_Orgnizer

#include "AlgoBase.h"
#include "rooCommon.h"
#include "TDOMParser.h"
#include "TXMLDocument.h"
#include "TXMLNode.h"
#include "TXMLAttr.h"
#include "TFile.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TString.h"
#include "asimovUtils.h"


class Orgnizer : public AlgoBase {
    public:
        Orgnizer() ;
        virtual void applyOptions(const boost::program_options::variables_map &vm) ;
        virtual void applyDefaultOptions() ;
        virtual void validateOptions() ;

        virtual const std::string & name() const {
          static const std::string name("Orgnizer");
          return name;
        }

        virtual bool run(bool makeSnapshot);

        virtual void readConfigXml( std::string configFileName );
        virtual void printSummary()
        {
          int nItems = (int)m_actionItems.size();
          for ( int i= 0; i < nItems; i++ ) {
            std::cout << "\tItem " << i << ", " << m_actionItems[i] << std::endl;
          }
        }

        // virtual TString replaceWithIndex(TString _formExpr)
        // {
        //         TString ret = "";
        //         if ( _formExpr.Contains("@") ) {
        //                 std::cout << "\tIt's already using index, formula: " << _formExpr << std::endl;
        //         }
        //         else{
        //                 TString newFormExpr = _formExpr;

        //                 /* first fill actualVars */
        //                 std::vector<TString> actualVars;
        //                 // <Item Name="expr::CGl2_HGlGl('@0*@0',CGl)"/>
        //                 /* SUPPOSED every var begins with "," */
        //                 TString expr  =  _formExpr.ReplaceAll(" ", "");
        //                 expr  =  expr.ReplaceAll("(", "");
        //                 expr  =  expr.ReplaceAll(")", "");
        //                 int index = 0;
        //                 size_t position  =  0;
        //                 while ( expr.Contains(",") ) {
        //                         position  =  expr.First(",");
        //                         TString subStr  =  expr(0, position); 
        //                         /* skip the first one */
        //                         if ( index>0 ) {
        //                                 /* the first is "," */
        //                                 if ( subStr.Contains("[") ) {
        //                                         subStr  =  subStr(0, subStr.First("[")).Data();
        //                                 }
        //                                 actualVars.push_back(subStr);
        //                                 std::cout << "\treal var1: " << subStr << std::endl;
        //                         }
        //                         size_t length  =  expr.Length();
        //                         expr  =  expr(position+1, length).Data();
        //                         index ++ ;
        //                 }
        //                 {
        //                         std::cout << "expr: " << expr << std::endl;
        //                         /* should add the last one */
        //                         TString subStr  =  expr.Data();
        //                         if ( subStr.Contains("[") ) {
        //                                 subStr  =  subStr(0, subStr.First("[")).Data();
        //                         }
        //                         actualVars.push_back(subStr);
        //                         std::cout << "\treal var2: " << subStr << std::endl;
        //                 }

        //                 std::map<std::string, std::string> reNameMap;
        //                 /* want to replace the long ones first, so that there is no mis-replacement */
        //                 std::vector<std::string> allOldNames;
        //                 std::vector<int> indice;
        //                 std::vector<int> nameLength;
        //                 int num = actualVars.size();
        //                 for ( int i= 0; i < num; i++ ) {
        //                         TString oldName = actualVars[i];
        //                         TString newName = TString::Format("@%d", i);
        //                         indice.push_back(i);
        //                         nameLength.push_back(oldName.Length());
        //                         allOldNames.push_back(oldName.Data());
        //                         reNameMap[oldName.Data()] = newName.Data();
        //                 }
        //                 TMath::Sort(num, &nameLength[0], &indice[0], true);

        //                 size_t firstP  =  newFormExpr.First(",");
        //                 TString firstPart  =  newFormExpr(0, firstP);
        //                 TString secondPart  =  newFormExpr(firstP+1, newFormExpr.Length());

        //                 for ( int i= 0; i < num; i++ ) {
        //                         int index = indice[i];
        //                         TString oldName = allOldNames[index];
        //                         TString newName = reNameMap[oldName.Data()].c_str();
        //                         firstPart = firstPart.ReplaceAll(oldName, newName);
        //                 }
        //                 std::cout << "\tWhat??? " << newFormExpr << std::endl;
        //                 ret = firstPart+secondPart;
        //         }
        //         return ret;
        // }


 private:
	std::vector<TString> SplitString(const TString& theOpt, const char separator );
        std::vector<std::string> m_actionItems;
        std::string m_inFile;
        std::string m_outFile;
        std::string m_modelName;
        std::vector<std::string> m_poiNames;

};

#endif

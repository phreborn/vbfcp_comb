/*
 * =====================================================================================
 *
 *       Filename:  AlgoBase.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  05/17/2012 02:49:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoshuang Ji (那你是真的牛逼), haoshuang.ji@cern.ch
 *   Organization:  University of Wisconsin
 *
 * Rewritten by Hongtao Yang (Hongtao.Yang@cern.ch) in 2019.
 *
 * =====================================================================================
 */


#ifndef Manager_AlgoBase_h
#define Manager_AlgoBase_h

#include <boost/program_options.hpp>
#include "CommonHead.h"

class AlgoBase
{
public:
  AlgoBase() { }
  AlgoBase( const char* desc ) : options_( desc ) { }

  virtual void applyOptions( const boost::program_options::variables_map& vm )
  { }
  virtual void applyDefaultOptions()
  { }
  virtual void validateOptions()
  { }

  virtual bool run() = 0;
  virtual const TString& name() const = 0;
  const boost::program_options::options_description& options() const
  {
    return options_;
  }

protected:
  boost::program_options::options_description options_;
};

#endif

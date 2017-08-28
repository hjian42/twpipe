#ifndef __TWPIPE_TOKENIZE_MODEL_BUILDER_H__
#define __TWPIPE_TOKENIZE_MODEL_BUILDER_H__

#include <boost/program_options.hpp>
#include "tokenize_model.h"

namespace po = boost::program_options;

namespace twpipe {

struct TokenizeModelBuilder {
  enum ModelType { 
    kLinearGRUTokenizeModel,
    kLinearLSTMTokenizeModel,
    kSegmentalGRUTokenizeModel,
    kSegmentalLSTMTokenizeModel
  };

  ModelType model_type;
  std::string model_name;
  unsigned char_size;
  unsigned char_dim;
  unsigned hidden_dim;
  unsigned n_layers;
  unsigned seg_dim;
  unsigned dur_dim;

  TokenizeModelBuilder(po::variables_map & conf);

  TokenizeModel * build(dynet::ParameterCollection & model);

  void to_json();

  TokenizeModel * from_json(dynet::ParameterCollection & model);
};

}

#endif  //  end for __TWPIPE_TOKENIZE_MODEL_BUILDER_H__

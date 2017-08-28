#include "postag_model_builder.h"
#include "twpipe/logging.h"
#include "twpipe/model.h"
#include "twpipe/alphabet_collection.h"

namespace twpipe {

template<> const char* CharacterGRUPostagModel::name = "CharacterGRUPostagModel";
template<> const char* CharacterLSTMPostagModel::name = "CharacterLSTMPostagModel";
template<> const char* CharacterGRUCRFPostagModel::name = "CharacterGRUCRFPostagModel";
template<> const char* CharacterLSTMCRFPostagModel::name = "CharacterLSTMCRFPostagModel";
template<> const char* CharacterGRUWithClusterPostagModel::name = "CharacterGRUWithClusterPostagModel";
template<> const char* CharacterLSTMWithClusterPostagModel::name = "CharacterLSTMWithClusterPostagModel";

PostagModelBuilder::PostagModelBuilder(po::variables_map & conf) {
  model_name = conf["pos-model-name"].as<std::string>();

  if (model_name == "char-gru") {
    model_type = kCharacterGRUPostagModel;
  } else if (model_name == "char-lstm") {
    model_type = kCharacterLSTMPostagModel;
  } else if (model_name == "char-gru-crf") {
    model_type = kCharacterGRUPostagCRFModel;
  } else if (model_name == "char-lstm-crf") {
    model_type = kCharacterLSTMPostagCRFModel;
  } else if (model_name == "char-gru-wcluster") {
    model_type = kCharacterClusterGRUPostagModel;
  } else if (model_name == "char-lstm-wcluster") {
    model_type = kCharacterClusterLSTMPostagModel;
  } else {
    _ERROR << "[postag|model_builder] unknow postag model: " << model_name;
  }

  char_size = AlphabetCollection::get()->char_map.size();
  pos_size = AlphabetCollection::get()->pos_map.size();
  char_dim = (conf.count("pos-char-dim") ? conf["pos-char-dim"].as<unsigned>() : 0);
  char_hidden_dim = (conf.count("pos-char-hidden-dim") ? conf["pos-char-hidden-dim"].as<unsigned>() : 0);
  char_n_layers = (conf.count("pos-char-n-layer") ? conf["pos-char-n-layer"].as<unsigned>() : 0);
  word_hidden_dim = (conf.count("pos-word-hidden-dim") ? conf["pos-word-hidden-dim"].as<unsigned>() : 0);
  word_n_layers = (conf.count("pos-word-n-layer") ? conf["pos-word-n-layer"].as<unsigned>() : 0);
  cluster_dim = (conf.count("pos-cluster-dim") ? conf["pos-cluster-dim"].as<unsigned>() : 0);
  cluster_hidden_dim = (conf.count("pos-cluster-hidden-dim") ? conf["pos-cluster-hidden-dim"].as<unsigned>() : 0);
  cluster_n_layers = (conf.count("pos-cluster-n-layer") ? conf["pos-cluster-n-layer"].as<unsigned>() : 0);
  pos_dim = (conf.count("pos-pos-dim") ? conf["pos-pos-dim"].as<unsigned>() : 0);
  embed_dim = (conf.count("embedding-dim") ? conf["embedding-dim"].as<unsigned>() : 0);
}

PostagModel * PostagModelBuilder::build(dynet::ParameterCollection & model) {
  PostagModel * engine = nullptr;
  if (model_type == kCharacterGRUPostagModel) {
    engine = new CharacterGRUPostagModel(model, char_size, char_dim, char_hidden_dim,
                                         char_n_layers, embed_dim, word_hidden_dim,
                                         word_n_layers, pos_dim);
  } else if (model_type == kCharacterLSTMPostagModel) {
    engine = new CharacterLSTMPostagModel(model, char_size, char_dim, char_hidden_dim,
                                          char_n_layers, embed_dim, word_hidden_dim,
                                          word_n_layers, pos_dim);
  } else if (model_type == kCharacterGRUPostagCRFModel) {
    engine = new CharacterGRUCRFPostagModel(model, char_size, char_dim, char_hidden_dim,
                                            char_n_layers, embed_dim, word_hidden_dim,
                                            word_n_layers, pos_dim);
  } else if (model_type == kCharacterLSTMPostagCRFModel) {
    engine = new CharacterLSTMCRFPostagModel(model, char_size, char_dim, char_hidden_dim,
                                             char_n_layers, embed_dim, word_hidden_dim,
                                             word_n_layers, pos_dim);
  } else if (model_type == kCharacterClusterGRUPostagModel) {
    engine = new CharacterGRUWithClusterPostagModel(model, char_size, char_dim, char_hidden_dim,
                                                    char_n_layers, embed_dim, word_hidden_dim,
                                                    word_n_layers, cluster_dim, cluster_hidden_dim,
                                                    cluster_n_layers, pos_dim);
  } else if (model_type == kCharacterClusterLSTMPostagModel) {
    engine = new CharacterLSTMWithClusterPostagModel(model, char_size, char_dim, char_hidden_dim,
                                                     char_n_layers, embed_dim, word_hidden_dim,
                                                     word_n_layers, cluster_dim, cluster_hidden_dim,
                                                     cluster_n_layers, pos_dim);
  } else {
    _ERROR << "[postag|model_builder] unknow postag model: " << model_name;
    exit(1);
  }
  return engine;
}

void PostagModelBuilder::to_json() {
  Model::get()->to_json(Model::kPostaggerName, {
    {"name", model_name},
    {"n-chars", boost::lexical_cast<std::string>(char_size)},
    {"char-dim", boost::lexical_cast<std::string>(char_dim)},
    {"char-hidden-dim", boost::lexical_cast<std::string>(char_hidden_dim)},
    {"char-n-layers", boost::lexical_cast<std::string>(char_n_layers)},
    {"word-hidden-dim", boost::lexical_cast<std::string>(word_hidden_dim)},
    {"word-n-layers", boost::lexical_cast<std::string>(word_n_layers)},
    {"pos-dim", boost::lexical_cast<std::string>(pos_dim)},
    {"n-postags", boost::lexical_cast<std::string>(pos_size)},
    {"emb-dim", boost::lexical_cast<std::string>(embed_dim)}
  });

  if (model_type == kCharacterClusterGRUPostagModel ||
      model_type == kCharacterClusterLSTMPostagModel) {
    Model::get()->to_json(Model::kPostaggerName, {
      {"cluster-dim", boost::lexical_cast<std::string>(char_dim)},
      {"cluster-hidden-dim", boost::lexical_cast<std::string>(char_hidden_dim)},
      {"cluster-n-layers", boost::lexical_cast<std::string>(char_n_layers)},
    });
  }
}

PostagModel * PostagModelBuilder::from_json(dynet::ParameterCollection & model) {
  PostagModel * engine = nullptr;

  Model * globals = Model::get();
  model_name = globals->from_json(Model::kPostaggerName, "name");
  unsigned temp_size;

  temp_size = 
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "n-chars"));
  if (char_size == 0) {
    char_size = temp_size;
  } else {
    BOOST_ASSERT_MSG(char_size == temp_size, "[postag|model_builder] char-size mismatch!");
  }

  temp_size =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "n-postags"));
  if (pos_size == 0) {
    pos_size = temp_size;
  } else {
    BOOST_ASSERT_MSG(pos_size == temp_size, "[postag|model_builder] pos-size mismatch!");
  }

  char_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "char-dim"));
  char_hidden_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "char-hidden-dim"));
  char_n_layers =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "char-n-layers"));
  word_hidden_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "word-hidden-dim"));
  word_n_layers =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "word-n-layers"));
  cluster_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "cluster-dim"));
  cluster_hidden_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "cluster-hidden-dim"));
  cluster_n_layers =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "cluster-n-layers"));
  pos_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "pos-dim"));
  embed_dim =
    boost::lexical_cast<unsigned>(globals->from_json(Model::kPostaggerName, "emb-dim"));

  if (model_name == "char-gru") {
    model_type = kCharacterGRUPostagModel;
    engine = new CharacterGRUPostagModel(model, char_size, char_dim, char_hidden_dim,
                                         char_n_layers, embed_dim, word_hidden_dim,
                                         word_n_layers, pos_dim);
  } else if (model_name == "char-lstm") {
    model_type = kCharacterLSTMPostagModel;
    engine = new CharacterLSTMPostagModel(model, char_size, char_dim, char_hidden_dim,
                                          char_n_layers, embed_dim, word_hidden_dim,
                                          word_n_layers, pos_dim);
  } else if (model_name == "char-gru-crf") {
    model_type = kCharacterGRUPostagCRFModel;
    engine = new CharacterGRUCRFPostagModel(model, char_size, char_dim, char_hidden_dim,
                                            char_n_layers, embed_dim, word_hidden_dim,
                                            word_n_layers, pos_dim);
  } else if (model_name == "char-lstm-crf") {
    model_type = kCharacterLSTMPostagCRFModel;
    engine = new CharacterLSTMCRFPostagModel(model, char_size, char_dim, char_hidden_dim,
                                             char_n_layers, embed_dim, word_hidden_dim,
                                             word_n_layers, pos_dim);
  } else if (model_name == "char-gru-wcluster") {
    model_type = kCharacterClusterGRUPostagModel;
    engine = new CharacterGRUWithClusterPostagModel(model, char_size, char_dim, char_hidden_dim,
                                                    char_n_layers, embed_dim, word_hidden_dim,
                                                    word_n_layers, cluster_dim, cluster_hidden_dim,
                                                    cluster_n_layers, pos_dim);
  } else if (model_name == "char-lstm-wcluster") {
    model_type = kCharacterClusterLSTMPostagModel;
    engine = new CharacterLSTMWithClusterPostagModel(model, char_size, char_dim, char_hidden_dim,
                                                     char_n_layers, embed_dim, word_hidden_dim,
                                                     word_n_layers, cluster_dim, cluster_hidden_dim,
                                                     cluster_n_layers, pos_dim);
  } else {
    _ERROR << "[postag|model_builder] unknow postag model: " << model_name;
    exit(1);
  }

  globals->from_json(Model::kPostaggerName, model);
  
  return engine;
}

}

#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "tokenizer/tokenize_model.h"
#include "tokenizer/tokenize_model_builder.h"
#include "tokenizer/tokenizer_trainer.h"
#include "postagger/postag_model.h"
#include "postagger/postag_model_builder.h"
#include "postagger/postagger_trainer.h"
#include "parser/parse_model.h"
#include "parser/parse_model_builder.h"
#include "parser/parser_trainer.h"
#include "twpipe/logging.h"
#include "twpipe/alphabet_collection.h"
#include "twpipe/corpus.h"
#include "twpipe/optimizer_builder.h"
#include "twpipe/trainer.h"
#include "twpipe/model.h"
#include "twpipe/embedding.h"
#include "twpipe/cluster.h"

namespace po = boost::program_options;

void init_command_line(int argc, char* argv[], po::variables_map& conf) {
  po::options_description generic_opts("Generic options");
  generic_opts.add_options()
    ("verbose,v", "Details logging.")
    ("help,h", "show help information.")
    ("train", "use to specify training.")
    ("input-file", po::value<std::string>(), "input files")
    ;

  po::options_description running_opts("Running options");
  running_opts.add_options()
    ("tokenize", "perform tokenization")
    ("postag", "perform tagging")
    ("parse", "perform parsing")
    ("format", po::value<std::string>()->default_value("plain"), "the format of input data [plain|conll].")
    ;

  po::options_description model_opts = twpipe::Model::get_options();
  po::options_description embed_opts = twpipe::WordEmbedding::get_options();
  po::options_description cluster_opts = twpipe::WordCluster::get_options();
  po::options_description training_opts = twpipe::Trainer::get_options();
  po::options_description tokenizer_opts = twpipe::TokenizeModel::get_options();
  po::options_description postagger_opts = twpipe::PostagModel::get_options();
  po::options_description parser_opts = twpipe::ParseModel::get_options();
  po::options_description parser_train_opts = twpipe::SupervisedTrainer::get_options();
  po::options_description optimizer_opts = twpipe::OptimizerBuilder::get_options();

  po::positional_options_description input_opts;
  input_opts.add("input-file", -1);

  po::options_description cmd("Usage: ./twpipe [running_opts] model_file [input_file]\n"
                              "       ./twpipe --train [training_opts] model_file [input_file]");
  cmd.add(generic_opts)
    .add(running_opts)
    .add(model_opts)
    .add(embed_opts)
    .add(cluster_opts)
    .add(training_opts)
    .add(tokenizer_opts)
    .add(postagger_opts)
    .add(parser_opts)
    .add(parser_train_opts)
    .add(optimizer_opts)
    ;

  po::store(po::command_line_parser(argc, argv).options(cmd).positional(input_opts).run(),
            conf);
  po::notify(conf);

  if (conf.count("help")) {
    std::cerr << cmd << std::endl;
    exit(1);
  }
  twpipe::init_boost_log(conf.count("verbose") > 0);
  
  if (!conf.count("input-file")) {
    std::cerr << "Please specify input file." << std::endl;
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  dynet::initialize(argc, argv);

  po::variables_map conf;
  init_command_line(argc, argv, conf);

  if (conf.count("embedding")) {
    twpipe::WordEmbedding::get()->load(conf["embedding"].as<std::string>(),
                                       conf["embedding-dim"].as<unsigned>());
  } else {
    twpipe::WordEmbedding::get()->empty(conf["embedding-dim"].as<unsigned>());
  }

  if (conf.count("cluster")) {
    twpipe::WordCluster::get()->load(conf["cluster"].as<std::string>());
  } else {
    twpipe::WordCluster::get()->empty();
  }

  if (conf.count("train")) {
    twpipe::Corpus corpus;
    corpus.load_training_data(conf["input-file"].as<std::string>());
    twpipe::AlphabetCollection::get()->to_json();

    if (conf.count("heldout")) {
      corpus.load_devel_data(conf["heldout"].as<std::string>());
    }

    twpipe::OptimizerBuilder opt_builder(conf);

    if (conf["train-tokenizer"].as<bool>() == true) {
      _INFO << "[twpipe] going to train tokenizer.";
      
      dynet::ParameterCollection model;

      twpipe::TokenizeModelBuilder builder(conf);
      builder.to_json();

      twpipe::TokenizeModel * engine = builder.build(model);
      twpipe::TokenizerTrainer trainer(*engine, opt_builder, conf);
      trainer.train(corpus);
    }
    if (conf["train-postagger"].as<bool>() == true) {
      _INFO << "[twpipe] going to train postagger.";

      dynet::ParameterCollection model;

      twpipe::PostagModelBuilder builder(conf);
      builder.to_json();

      twpipe::PostagModel * engine = builder.build(model);
      twpipe::PostaggerTrainer trainer(*engine, opt_builder, conf);
      trainer.train(corpus);
    }
    if (conf["train-parser"].as<bool>() == true) {
      _INFO << "[twpipe] going to train parser.";

      dynet::ParameterCollection model;
      twpipe::ParseModelBuilder builder(conf);
      builder.to_json();

      twpipe::ParseModel * engine = builder.build(model);
      twpipe::SupervisedTrainer trainer((*engine), opt_builder, conf);
      trainer.train(corpus);
    }

    std::string model_name = conf["model"].as<std::string>();
    twpipe::Model::get()->save(model_name);
  } else {
    std::string model_name = conf["model"].as<std::string>();
    twpipe::Model::get()->load(model_name);
    twpipe::AlphabetCollection::get()->from_json();

    if (conf["format"].as<std::string>() == "plain") {
      twpipe::TokenizeModel * tok_engine = nullptr;
      twpipe::PostagModel * pos_engine = nullptr;
        
      dynet::ParameterCollection tok_model;
      dynet::ParameterCollection pos_model;

      bool load_tokenize_model = (conf.count("tokenize") || conf.count("postag") || conf.count("parse"));
      bool load_postag_model = (conf.count("postag") || conf.count("parse"));
      bool load_parse_model = (conf.count("parse"));

      if (load_tokenize_model) {
        if (!twpipe::Model::get()->has_tokenizer_model()) {
          _ERROR << "[twpipe] doesn't have tokenizer model!";
          exit(1);
        }
        twpipe::TokenizeModelBuilder tok_builder(conf);
        tok_engine = tok_builder.from_json(tok_model);
      } 
      if (load_postag_model) {
        if (!twpipe::Model::get()->has_postagger_model()) {
          _ERROR << "[twpipe] doesn't have postagger model!";
          exit(1);
        }
        twpipe::PostagModelBuilder pos_builder(conf);
        pos_engine = pos_builder.from_json(pos_model);
      }
      if (load_parse_model) {
        BOOST_ASSERT_MSG(false, "[twpipe] parser not implemented.");
      }

      std::string buffer;
      std::ifstream ifs(conf["input-file"].as<std::string>());
      while (std::getline(ifs, buffer)) {
        boost::algorithm::trim(buffer);
        std::vector<std::string> tokens;
        std::vector<std::string> postags;
        if (tok_engine) {
          tok_engine->tokenize(buffer, tokens);
        }

        if (pos_engine) {
          pos_engine->postag(tokens, postags);
        }

        std::cout << "# text = " << buffer << "\n";
        for (unsigned i = 0; i < tokens.size(); ++i) {
          std::cout << i + 1 << "\t" << tokens[i] << "\t_\t"
            << (pos_engine ? postags[i] : "_") << "\t_\t_\t_\t_\t_\n";
        }
        std::cout << "\n";
      }
    } else {
      // for conll format, tokenization is impossible.
      twpipe::PostagModel * pos_engine = nullptr;

      dynet::ParameterCollection pos_model;

      bool load_postag_model = (conf.count("postag") || conf.count("parse"));
      bool load_parse_model = (conf.count("parse"));
      
      if (load_postag_model) {
        if (!twpipe::Model::get()->has_postagger_model()) {
          _ERROR << "[twpipe] doesn't have postagger model!";
          exit(1);
        }
        twpipe::PostagModelBuilder pos_builder(conf);
        pos_engine = pos_builder.from_json(pos_model);
      }
  
      std::vector<std::string> tokens;
      std::vector<std::string> gold_postags;
      std::vector<std::string> postags;
      std::string sentence;
      std::string buffer;
      std::ifstream ifs(conf["input-file"].as<std::string>());
      float n_pos_corr = 0.;
      float n_total = 0.;
      while (std::getline(ifs, buffer)) {
        boost::algorithm::trim(buffer);
        if (buffer.size() == 0) {
          if (pos_engine) {
            pos_engine->postag(tokens, postags);
          }

          std::cout << "# text = " << sentence << "\n";
          for (unsigned i = 0; i < tokens.size(); ++i) {
            std::cout << i + 1 << "\t" << tokens[i] << "\t_\t"
              << postags[i] << "\t_\tGoldPOS=" << gold_postags[i] << "\t_\t_\t_\n";
            if (postags[i] == gold_postags[i]) { n_pos_corr += 1.; }
            n_total += 1.;
          }
          std::cout << "\n";

          tokens.clear();
          postags.clear();
          gold_postags.clear();
        } else if (buffer[0] == '#') {
          if (boost::algorithm::starts_with(buffer, "# text = ")) { sentence = buffer.substr(9); }
        } else {
          std::vector<std::string> data;
          boost::algorithm::split(data, buffer, boost::is_any_of("\t "));
          tokens.push_back(data[1]);
          gold_postags.push_back(data[3]);
        }
      }
      _INFO << "[evaluate] postag accuracy: " << n_pos_corr / n_total;
    }
  }
  return 0;
}

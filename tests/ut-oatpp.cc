/**
 * DeepDetect
 * Copyright (c) 2020 Jolibrain SASU
 * Author: Mehdi Abaakouk <mabaakouk@jolibrain.com>
 *
 * This file is part of deepdetect.
 *
 * deepdetect is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * deepdetect is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with deepdetect.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <gtest/gtest.h>

#include "oatpp-test/UnitTest.hpp"

#include "ut-oatpp.h"

const std::string serv
    = "very_long_label_service_name_with_😀_inside_and_some_MAJ";
const std::string serv_lower
    = "very_long_label_service_name_with_😀_inside_and_some_maj";
const std::string serv2 = "myserv2";
#if defined(CPU_ONLY) || defined(USE_CAFFE_CPU_ONLY)
static std::string iterations_mnist = "10";
#else
static std::string iterations_mnist = "10000";
#endif

void test_info(std::shared_ptr<DedeApiTestClient> client)
{
  auto response = client->get_info();
  ASSERT_EQ(response->getStatusCode(), 200);
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);

  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_TRUE(d.HasMember("head"));
  ASSERT_TRUE(d["head"].HasMember("services"));
  ASSERT_EQ(0, d["head"]["services"].Size());
}

void test_services(std::shared_ptr<DedeApiTestClient> client)
{
  std::string serv_put
      = "{\"mllib\":\"caffe\",\"description\":\"example classification "
        "service\",\"type\":\"supervised\",\"parameters\":{\"input\":{"
        "\"connector\":\"csv\"},\"mllib\":{\"template\":\"mlp\",\"nclasses\":"
        "2,"
        "\"layers\":[50,50,50],\"activation\":\"PReLU\"}},\"model\":{"
        "\"templates\":\"../templates/caffe/\",\"repository\":\".\"}}";

  auto response = client->put_services(serv.c_str(), serv_put.c_str());
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);

  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // service info
  response = client->get_services(serv.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_TRUE(d.HasMember("body"));
  ASSERT_EQ("caffe", d["body"]["mllib"]);
  ASSERT_EQ(serv_lower.c_str(), d["body"]["name"]);
  ASSERT_TRUE(d["body"].HasMember("jobs"));

  // info call
  response = client->get_info();
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d["head"].HasMember("services"));
  ASSERT_EQ(1, d["head"]["services"].Size());

  // delete call
  response = client->delete_services(serv.c_str(), nullptr);
  ASSERT_EQ(response->getStatusCode(), 200);

  // info call
  response = client->get_info();
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d["head"].HasMember("services"));
  ASSERT_EQ(0, d["head"]["services"].Size());

  // service info
  response = client->get_services(serv.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 404);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
}

void test_train(std::shared_ptr<DedeApiTestClient> client)
{
  // service definition
  std::string mnist_repo = "../examples/caffe/mnist/";
  std::string serv_put2
      = "{\"mllib\":\"caffe\",\"description\":\"my "
        "classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\""
        + mnist_repo
        + "\"},\"parameters\":{\"input\":{\"connector\":\"image\"},\"mllib\":{"
          "\"nclasses\":10}}}";

  // service creation
  auto response = client->put_services(serv.c_str(), serv_put2.c_str());
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // train blocking
  std::string train_post = "{\"service\":\"" + serv
                           + "\",\"async\":false,\"parameters\":{\"mllib\":{"
                             "\"gpu\":true,\"solver\":{\"iterations\":"
                           + iterations_mnist + "}}}}";
  response = client->post_train(train_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d.HasMember("body"));
  ASSERT_TRUE(d["body"].HasMember("measure"));
#if defined(CPU_ONLY) || defined(USE_CAFFE_CPU_ONLY)
  ASSERT_EQ(9, d["body"]["measure"]["iteration"].GetDouble());
#else
  ASSERT_EQ(9999, d["body"]["measure"]["iteration"].GetDouble());
#endif
  ASSERT_TRUE(fabs(d["body"]["measure"]["train_loss"].GetDouble()) > 0.0);

  // remove service and trained model files
  response = client->delete_services(serv.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);

  // service creation
  response = client->put_services(serv.c_str(), serv_put2.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // train async
  train_post = "{\"service\":\"" + serv
               + "\",\"async\":true,\"parameters\":{\"mllib\":{\"gpu\":true,"
                 "\"solver\":{\"iterations\":"
               + iterations_mnist + "}}}}";
  response = client->post_train(train_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasMember("body"));

  sleep(1);

  // service info
  response = client->get_services(serv.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_TRUE(d.HasMember("body"));
  ASSERT_EQ("caffe", d["body"]["mllib"]);
  ASSERT_EQ(serv_lower.c_str(), d["body"]["name"]);
  ASSERT_TRUE(d["body"].HasMember("jobs"));
  ASSERT_EQ("running", d["body"]["jobs"][0]["status"]);

  // get info on training job
  bool running = true;
  while (running)
    {
      response = client->get_train(serv.c_str(), 1, 1, 100);
      message = response->readBodyToString();
      ASSERT_TRUE(message != nullptr);
      std::string jstr = message;
      running = jstr.find("running") != std::string::npos;
      if (running)
        {
          std::cerr << "jstr=" << jstr << std::endl;
          JDoc jd2;
          jd2.Parse<rapidjson::kParseNanAndInfFlag>(jstr.c_str());
          ASSERT_TRUE(!jd2.HasParseError());
          ASSERT_TRUE(jd2.HasMember("status"));
          ASSERT_EQ(200, jd2["status"]["code"]);
          ASSERT_EQ("OK", jd2["status"]["msg"]);
          ASSERT_TRUE(jd2.HasMember("head"));
          ASSERT_EQ("/train", jd2["head"]["method"]);
          ASSERT_TRUE(jd2["head"]["time"].GetDouble() >= 0);
          ASSERT_EQ("running", jd2["head"]["status"]);
          ASSERT_EQ(1, jd2["head"]["job"]);
          ASSERT_TRUE(jd2.HasMember("body"));
          ASSERT_TRUE(jd2["body"]["measure"].HasMember("train_loss"));
          ASSERT_TRUE(fabs(jd2["body"]["measure"]["train_loss"].GetDouble())
                      > 0);
          ASSERT_TRUE(jd2["body"]["measure"].HasMember("iteration"));
          ASSERT_TRUE(jd2["body"]["measure"]["iteration"].GetDouble() >= 0);
          ASSERT_TRUE(jd2["body"].HasMember("measure_hist"));
          ASSERT_TRUE(
              100 >= jd2["body"]["measure_hist"]["train_loss_hist"].Size());
        }
      else
        ASSERT_TRUE(jstr.find("finished") != std::string::npos);
    }

  // remove service and trained model files
  response = client->delete_services(serv.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
}

void test_multiservices(std::shared_ptr<DedeApiTestClient> client)
{
  // service definition
  std::string mnist_repo = "../examples/caffe/mnist/";
  std::string serv_put2
      = "{\"mllib\":\"caffe\",\"description\":\"my "
        "classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\""
        + mnist_repo
        + "\"},\"parameters\":{\"input\":{\"connector\":\"image\"},\"mllib\":{"
          "\"nclasses\":10}}}";

  // service creation
  auto response = client->put_services(serv.c_str(), serv_put2.c_str());
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // service creation
  response = client->put_services(serv2.c_str(), serv_put2.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // info call
  response = client->get_info();
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  std::string jstr = message;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(jstr.c_str());
  ASSERT_TRUE(d["head"].HasMember("services"));
  ASSERT_EQ(2, d["head"]["services"].Size());
  ASSERT_EQ(serv2, d["head"]["services"][0]["name"].GetString());
  ASSERT_EQ(serv_lower, d["head"]["services"][1]["name"].GetString());

  // remove services and trained model files
  response = client->delete_services(serv.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
  response = client->delete_services(serv2.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
}

void test_concurrency(std::shared_ptr<DedeApiTestClient> client)
{
  // service definition
  std::string mnist_repo = "../examples/caffe/mnist/";
  std::string serv_put2
      = "{\"mllib\":\"caffe\",\"description\":\"my "
        "classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\""
        + mnist_repo
        + "\"},\"parameters\":{\"input\":{\"connector\":\"image\"},\"mllib\":{"
          "\"nclasses\":10}}}";

  // service creation
  auto response = client->put_services(serv.c_str(), serv_put2.c_str());
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // train async
  std::string train_post
      = "{\"service\":\"" + serv
        + "\",\"async\":true,\"parameters\":{\"mllib\":{\"gpu\":true,"
          "\"solver\":{\"iterations\": "
        + iterations_mnist + "}}}}";
  response = client->post_train(train_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasMember("body"));

  // service creation
  response = client->put_services(serv2.c_str(), serv_put2.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // info call
  response = client->get_info();
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::string jstr = message;
  std::cout << "jstr=" << jstr << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d = rapidjson::Document();
  d.Parse<rapidjson::kParseNanAndInfFlag>(jstr.c_str());
  ASSERT_TRUE(d["head"].HasMember("services"));
  ASSERT_EQ(2, d["head"]["services"].Size());
  ASSERT_EQ(serv2, d["head"]["services"][0]["name"].GetString());
  ASSERT_EQ(serv_lower, d["head"]["services"][1]["name"].GetString());

  // train async second job
  train_post = "{\"service\":\"" + serv2
               + "\",\"async\":true,\"parameters\":{\"mllib\":{\"gpu\":true,"
                 "\"solver\":{\"iterations\":"
               + iterations_mnist + "}}}}";
  response = client->post_train(train_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasMember("body"));

  sleep(1);

  // get info on job2
  response = client->get_train(serv2.c_str(), 1, nullptr, nullptr);
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);

  // get info on first training job
  int tmax = 10, t = 0;
  bool running = true;
  while (running)
    {
      response = client->get_train(serv2.c_str(), 1, 1, nullptr);
      message = response->readBodyToString();
      ASSERT_TRUE(message != nullptr);
      std::string jstr = message;
      running = jstr.find("running") != std::string::npos;
      if (!running)
        {
          std::cerr << "jstr=" << jstr << std::endl;
          JDoc jd2;
          jd2.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
          ASSERT_TRUE(!jd2.HasParseError());
          ASSERT_TRUE(jd2.HasMember("status"));
          ASSERT_EQ(200, jd2["status"]["code"]);
          ASSERT_EQ("OK", jd2["status"]["msg"]);
          ASSERT_TRUE(jd2.HasMember("head"));
          ASSERT_EQ("/train", jd2["head"]["method"]);
          ASSERT_TRUE(jd2["head"]["time"].GetDouble() >= 0);
          ASSERT_EQ("finished", jd2["head"]["status"]);
          ASSERT_EQ(1, jd2["head"]["job"]);
          ASSERT_TRUE(jd2.HasMember("body"));
          ASSERT_TRUE(jd2["body"]["measure"].HasMember("train_loss"));
          ASSERT_TRUE(fabs(jd2["body"]["measure"]["train_loss"].GetDouble())
                      > 0);
          ASSERT_TRUE(jd2["body"]["measure"].HasMember("iteration"));
          ASSERT_TRUE(jd2["body"]["measure"]["iteration"].GetDouble() > 0);
        }
      ++t;
      if (t > tmax)
        break;
    }

  // delete job1
  response = client->delete_train(serv.c_str(), 1);
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d.HasMember("head"));
  ASSERT_TRUE(d["head"].HasMember("status"));
  ASSERT_EQ("terminated", d["head"]["status"]);

  sleep(1);

  // get info on job1
  response = client->get_train(serv.c_str(), 1, nullptr, nullptr);
  ASSERT_EQ(response->getStatusCode(), 404);

  // delete job2
  response = client->delete_train(serv2.c_str(), 1);
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(d.HasMember("head"));
  ASSERT_TRUE(d["head"].HasMember("status"));
  ASSERT_EQ("terminated", d["head"]["status"]);

  sleep(1);

  // get info on job2
  response = client->get_train(serv2.c_str(), 1, nullptr, nullptr);
  ASSERT_EQ(response->getStatusCode(), 404);

  // remove services and trained model files
  response = client->delete_services(serv.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
  response = client->delete_services(serv2.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
}

void test_predict(std::shared_ptr<DedeApiTestClient> client)
{
  std::string mnist_repo = "../examples/caffe/mnist/";
  std::string serv_put2
      = "{\"mllib\":\"caffe\",\"description\":\"my "
        "classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\""
        + mnist_repo
        + "\"},\"parameters\":{\"input\":{\"connector\":\"image\"},\"mllib\":{"
          "\"nclasses\":10}}}";

  // service creation
  std::string jstr;

  auto response = client->put_services(serv.c_str(), serv_put2.c_str());
  auto message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  rapidjson::Document d;
  d.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_FALSE(d.HasParseError());
  ASSERT_TRUE(d.HasMember("status"));
  ASSERT_EQ(201, d["status"]["code"].GetInt());

  // train sync
  std::string train_post
      = "{\"service\":\"" + serv
        + "\",\"async\":false,\"parameters\":{\"mllib\":{\"gpu\":true,"
          "\"solver\":{\"iterations\":"
        + iterations_mnist + ",\"snapshot_prefix\":\"" + mnist_repo
        + "/mylenet\"}},\"output\":{\"measure_hist\":true}}}";

  response = client->post_train(train_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  jstr = message;
  std::cout << "jstr=" << jstr << std::endl;
  ASSERT_EQ(response->getStatusCode(), 201);
  JDoc jd;
  jd.Parse<rapidjson::kParseNanAndInfFlag>(jstr.c_str());
  ASSERT_TRUE(!jd.HasParseError());
  ASSERT_TRUE(jd.HasMember("status"));
  ASSERT_EQ(201, jd["status"]["code"].GetInt());
  ASSERT_EQ("Created", jd["status"]["msg"]);
  ASSERT_TRUE(jd.HasMember("head"));
  ASSERT_EQ("/train", jd["head"]["method"]);
  ASSERT_TRUE(jd["head"]["time"].GetDouble() >= 0);
  ASSERT_TRUE(jd.HasMember("body"));
  ASSERT_TRUE(jd["body"].HasMember("measure"));
  ASSERT_TRUE(fabs(jd["body"]["measure"]["train_loss"].GetDouble()) >= 0);
  ASSERT_TRUE(jd["body"]["measure_hist"]["train_loss_hist"].Size() > 0);

  // predict
  std::string predict_post
      = "{\"service\":\"" + serv
        + "\",\"parameters\":{\"mllib\":{\"gpu\":true},\"input\":{\"bw\":true,"
          "\"width\":28,\"height\":28},\"output\":{\"best\":3}},\"data\":[\""
        + mnist_repo + "/sample_digit.png\",\"" + mnist_repo
        + "/sample_digit2.png\"]}";
  response = client->post_predict(predict_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);
  jstr = message;
  jd.Parse<rapidjson::kParseNanAndInfFlag>(message->c_str());
  ASSERT_TRUE(!jd.HasParseError());
  ASSERT_EQ(200, jd["status"]["code"]);
  ASSERT_TRUE(jd.HasMember("head"));
  ASSERT_EQ(serv_lower.c_str(), jd["head"]["service"]);
  ASSERT_EQ("/predict", jd["head"]["method"]);
  ASSERT_TRUE(jd["body"]["predictions"][0]["classes"][0]["prob"].GetDouble()
              > 0);
  ASSERT_TRUE(jd["body"]["predictions"][1]["classes"][0]["prob"].GetDouble()
              > 0);

  // predict with output template
  std::string ot
      = "{{#status}}{{code}}{{/"
        "status}}\\n{{#body}}{{#predictions}}*{{uri}}:\\n{{#classes}}{{cat}}->"
        "{{prob}}\\n{{/classes}}{{/predictions}}{{/body}}";
  predict_post
      = "{\"service\":\"" + serv
        + "\",\"parameters\":{\"mllib\":{\"gpu\":true},\"input\":{\"bw\":true,"
          "\"width\":28,\"height\":28},\"output\":{\"best\":3,\"template\":\""
        + ot + "\"}},\"data\":[\"" + mnist_repo + "/sample_digit.png\",\""
        + mnist_repo + "/sample_digit2.png\"]}";

  response = client->post_predict(predict_post.c_str());
  message = response->readBodyToString();
  ASSERT_TRUE(message != nullptr);
  std::cout << "jstr=" << *message << std::endl;
  ASSERT_EQ(response->getStatusCode(), 200);

  // remove services and trained model files
  response = client->delete_services(serv.c_str(), "lib");
  ASSERT_EQ(response->getStatusCode(), 200);
}

#define OATPP_DEDE_TEST(FUNC)                                                 \
  TEST(oatpp_jsonapi, FUNC)                                                   \
  {                                                                           \
    oatpp::base::Environment::init();                                         \
    DedeControllerTest *test = new DedeControllerTest(#FUNC, FUNC);           \
    test->run(1);                                                             \
    delete test;                                                              \
    oatpp::base::Environment::destroy();                                      \
  }

OATPP_DEDE_TEST(test_info);

#ifdef USE_CAFFE

OATPP_DEDE_TEST(test_services);
OATPP_DEDE_TEST(test_train);
OATPP_DEDE_TEST(test_multiservices);
OATPP_DEDE_TEST(test_concurrency);
OATPP_DEDE_TEST(test_predict);

#endif

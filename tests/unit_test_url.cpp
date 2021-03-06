/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
        * Neither the name of FastoGT. nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <gtest/gtest.h>

#include <common/uri/url.h>

#define DEV_VIDEO_PATH "/dev/video3"

TEST(Upath, isValidPathAndQuery) {
  common::uri::Upath path;
  ASSERT_FALSE(path.IsValid());

  const std::string file_name = "/index.html";
  common::uri::Upath path2(file_name);
  ASSERT_TRUE(path2.IsValid());
  ASSERT_EQ(file_name, path2.GetUpath());

  const std::string file_path = "/example.html";
  const std::string query = "var=This+is+a+simple+%26+short+test";
  common::uri::Upath path3(file_path + "?" + query);
  ASSERT_TRUE(path3.IsValid());
  ASSERT_EQ(file_path, path3.GetPath());
  // const char* dec = common::uri::detail::uri_decode(query.c_str(), query.length());
  // ASSERT_EQ(dec, path3.Query());

  // invalid Upath
  const std::string invalid2 = "..";
  common::uri::Upath pathInvalid(invalid2);
  ASSERT_FALSE(pathInvalid.IsValid());

  const std::string invalid3 = "../";
  pathInvalid = common::uri::Upath(invalid3);
  ASSERT_FALSE(pathInvalid.IsValid());

  const std::string invalid4 = "/../";
  pathInvalid = common::uri::Upath(invalid4);
  ASSERT_FALSE(pathInvalid.IsValid());

  const std::string invalid5 = "/..";
  pathInvalid = common::uri::Upath(invalid5);
  ASSERT_FALSE(pathInvalid.IsValid());
}

TEST(Url, IsValid) {
  common::uri::Url path;
  ASSERT_FALSE(path.IsValid());

  common::uri::Url path2("http://www.permadi.com/index.html");
  ASSERT_TRUE(path2.IsValid());

  common::uri::Url path3("http://www.permadi.com/tutorial/urlEncoding/index.html");
  ASSERT_TRUE(path3.IsValid());

  common::uri::Url path4(
      "http://www.permadi.com/tutorial/urlEncoding/"
      "example.html?var=This+is+a+simple+%26+short+test");
  ASSERT_TRUE(path4.IsValid());
}

TEST(Url, Scheme) {
  common::uri::Url http_uri_gstreamer("http://localhost:8000/master.m3u8");
  ASSERT_EQ(http_uri_gstreamer.GetScheme(), common::uri::Url::http);
  ASSERT_EQ(http_uri_gstreamer.GetHost(), "localhost:8000");
  common::uri::Upath http_path = http_uri_gstreamer.GetPath();
  ASSERT_TRUE(http_path.IsValid());
  ASSERT_EQ(http_path.GetHpath(), "/");
  ASSERT_EQ(http_path.GetFileName(), "master.m3u8");
  ASSERT_EQ(http_path.GetPath(), http_path.GetHpath() + http_path.GetFileName());

  common::uri::Url http_uri("http://localhost:8080/hls/69_avformat_test_alex_2/play.m3u8");
  ASSERT_EQ(http_uri.GetScheme(), common::uri::Url::http);
  ASSERT_EQ(http_uri.GetHost(), "localhost:8080");
  http_path = http_uri.GetPath();
  ASSERT_EQ(http_path.GetHpath(), "/hls/69_avformat_test_alex_2/");
  ASSERT_EQ(http_path.GetPath(), "/hls/69_avformat_test_alex_2/play.m3u8");
  ASSERT_EQ(http_path.GetFileName(), "play.m3u8");
  ASSERT_EQ(http_path.GetUpath(), "/hls/69_avformat_test_alex_2/play.m3u8");
  ASSERT_EQ(http_path.GetPath(), http_path.GetHpath() + http_path.GetFileName());

  common::uri::Url copy_http_uri = http_uri;
  common::uri::Upath up("/hls/69_avformat_test_alex_2/play.m3u8");
  ASSERT_EQ(up.GetUpath(), "/hls/69_avformat_test_alex_2/play.m3u8");
  ASSERT_EQ(up.GetPath(), "/hls/69_avformat_test_alex_2/play.m3u8");
  copy_http_uri.SetPath(up);
  ASSERT_EQ(copy_http_uri.GetUrl(), "http://localhost:8080/hls/69_avformat_test_alex_2/play.m3u8");

  common::uri::Url ftp_uri("ftp://localhost:8080");
  ASSERT_EQ(ftp_uri.GetScheme(), common::uri::Url::ftp);
  ASSERT_EQ(ftp_uri.GetHost(), "localhost:8080");
  common::uri::Upath ftp_path = ftp_uri.GetPath();
  ASSERT_EQ(ftp_path.GetHpath(), std::string());
  ASSERT_EQ(ftp_path.GetPath(), std::string());
  ASSERT_EQ(ftp_path.GetFileName(), std::string());
  ASSERT_EQ(ftp_path.GetUpath(), std::string());
  ASSERT_EQ(ftp_path.GetPath(), ftp_path.GetHpath() + ftp_path.GetFileName());

  common::uri::Url file_uri("file:///home/sasha/2.txt");
  ASSERT_EQ(file_uri.GetScheme(), common::uri::Url::file);
  ASSERT_EQ(file_uri.GetPath(), common::uri::Upath("/home/sasha/2.txt"));
  ASSERT_EQ(file_uri.GetHost(), std::string());

  common::uri::Url invalid_uri("home://user/logo.png");
  ASSERT_FALSE(invalid_uri.IsValid());

  common::uri::Url ws_uri("ws://localhost:8080");
  ASSERT_EQ(ws_uri.GetScheme(), common::uri::Url::ws);
  ASSERT_EQ(ws_uri.GetHost(), "localhost:8080");

  common::uri::Url udp_uri("udp://localhost:8080");
  ASSERT_EQ(udp_uri.GetScheme(), common::uri::Url::udp);
  ASSERT_EQ(udp_uri.GetHost(), "localhost:8080");

  common::uri::Url rtmp_uri("rtmp://localhost:8080");
  ASSERT_EQ(rtmp_uri.GetScheme(), common::uri::Url::rtmp);
  ASSERT_EQ(rtmp_uri.GetHost(), "localhost:8080");

  common::uri::Url dev_uri("dev://" DEV_VIDEO_PATH);
  ASSERT_EQ(dev_uri.GetScheme(), common::uri::Url::dev);
  ASSERT_EQ(dev_uri.GetPath(), common::uri::Upath(DEV_VIDEO_PATH));
  ASSERT_EQ(dev_uri.GetHost(), std::string());

  common::uri::Url rtsp_uri("rtsp://some.server/url");
  ASSERT_EQ(rtsp_uri.GetScheme(), common::uri::Url::rtsp);
  ASSERT_EQ(rtsp_uri.GetHost(), "some.server");

  common::uri::Url srt_uri("srt://localhost:8080");
  ASSERT_EQ(srt_uri.GetScheme(), common::uri::Url::srt);
  ASSERT_EQ(srt_uri.GetHost(), "localhost:8080");
}

TEST(Url, level) {
  common::uri::Url http_uri("http://localhost:8080/hls/69_avformat_test_alex_2/play.m3u8");
  common::uri::Upath p = http_uri.GetPath();
  size_t lv = p.GetLevels();
  ASSERT_EQ(lv, 2);
  std::string l1 = p.GetHpathLevel(1);
  ASSERT_EQ(l1, "hls/");
  std::string l2 = p.GetHpathLevel(2);
  ASSERT_EQ(l2, "hls/69_avformat_test_alex_2/");
}

TEST(Url, urlEncode) {
  const std::string url = "https://mywebsite/docs/english/site/mybook.do";
  char* enc = common::uri::detail::uri_encode(url.c_str(), url.length());
  ASSERT_STREQ(enc, "https%3A%2F%2Fmywebsite%2Fdocs%2Fenglish%2Fsite%2Fmybook.do");
  char* dec = common::uri::detail::uri_decode(enc, strlen(enc));
  ASSERT_STREQ(dec, url.c_str());
  free(enc);
  free(dec);
}

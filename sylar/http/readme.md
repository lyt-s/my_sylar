 sudo apt install ragel
 ragel -G -C httpclient_parser.rl -o httpclient_parser.cc
 ragel -G2 -C httpclient_parser.rl -o httpclient_parser.cc
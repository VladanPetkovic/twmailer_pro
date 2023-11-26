# Compiler and Compile options
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Werror -g -Wextra ##-lldap -llber

# LDAP library
LIBS = -lldap -llber

# Targets to build
all: twmailer-client twmailer-server

# Using automatic variables:
# $@ is the target name
# $^ is the name of all the prerequisites with spaces between them
# $< is the name of the first prerequisite

# .cpp files into .o object files and then links these object files to produce the executables
twmailer-client: client-main.o twmailer-client.o
	$(CXX) $(CXXFLAGS) obj/client-main.o obj/twmailer-client.o -o $@

twmailer-server: server-main.o twmailer-server.o ldap_fh.o
	$(CXX) $(CXXFLAGS) obj/server-main.o obj/twmailer-server.o obj/ldap_fh.o -o $@ $(LIBS)

client-main.o: src/client-main.cpp
	$(CXX) $(CXXFLAGS) -c src/client-main.cpp -o obj/client-main.o

server-main.o: src/server-main.cpp
	$(CXX) $(CXXFLAGS) -c src/server-main.cpp -o obj/server-main.o

twmailer-client.o: src/twmailer-client.cpp include/twmailer-client.h
	$(CXX) $(CXXFLAGS) -c $< -o obj/twmailer-client.o

twmailer-server.o: src/twmailer-server.cpp include/twmailer-server.h
	$(CXX) $(CXXFLAGS) -c $< -o obj/twmailer-server.o

ldap_fh.o: src/ldap_fh.cpp include/ldap_fh.h
	$(CXX) $(CXXFLAGS) -c $< -o obj/ldap_fh.o

clean:
	rm -f twmailer-client twmailer-server obj/*.o

.PHONY: all clean

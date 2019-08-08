SOURCE_DIR =./src
BUILD_DIR =./build
INSTALL_DIR =/var/www/fcgi/app

PROJECT_NAME =fcgi_redir
PROJECT_FILES =$(SOURCE_DIR)/main.c $(SOURCE_DIR)/cgi_handler_thread.c \
$(SOURCE_DIR)/advanced_string.c $(SOURCE_DIR)/cgi_query_parser.c $(SOURCE_DIR)/config_parser.c

DEPENDENCIES =-lfcgi -lpthread -lmemcached -lmysqlclient \
`mysql_config --cflags` `mysql_config --libs`

GCC_OPTIONS =-std=c11 -Wall

all: build_release

install: install_release
release: build_release install_release
debug: build_debug install_debug

build_release:
	mkdir -p $(BUILD_DIR)
	gcc -O2 $(GCC_OPTIONS) -o $(BUILD_DIR)/$(PROJECT_NAME) \
$(PROJECT_FILES) \
$(DEPENDENCIES)
	cp -r $(SOURCE_DIR)/config $(BUILD_DIR)/

build_debug:
	mkdir -p $(BUILD_DIR)
	gcc -g $(GCC_OPTIONS) -o $(BUILD_DIR)/$(PROJECT_NAME)_debug \
$(PROJECT_FILES) \
$(DEPENDENCIES)
	cp -r $(SOURCE_DIR)/config $(BUILD_DIR)/

install_release:
	mkdir -p $(INSTALL_DIR)
	cp $(BUILD_DIR)/$(PROJECT_NAME) $(INSTALL_DIR)/$(PROJECT_NAME)
	cp -r $(BUILD_DIR)/config $(INSTALL_DIR)/

install_debug:
	mkdir -p $(INSTALL_DIR)
	cp $(BUILD_DIR)/$(PROJECT_NAME)_debug $(INSTALL_DIR)/$(PROJECT_NAME)
	cp -r $(BUILD_DIR)/config $(INSTALL_DIR)/

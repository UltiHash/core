TARGET_NAME := UH-closed-beta-$(shell printf "%s" `date +"%Y%m%d"`).tar.bz2

.PHONY: all clean

all: release

release: docker-compose.yml.template prometheus.yml setup.yml ../cmake-build-release/client-shell/uh-cli ../cmake-build-release/client-sdk/libUDB_static.a ../client-sdk/include ../client-sdk/examples ./UhSandbox
	mkdir -p lib
	mkdir -p bin
	cp ../cmake-build-release/client-sdk/libUDB_static.a lib/
	cp ../cmake-build-release/client-shell/uh-cli bin/
	cp -r ../client-sdk/include ./
	cp -r ../client-sdk/examples ./
	tar -cjf $(TARGET_NAME) docker-compose.yml.template prometheus.yml setup.yml lib include examples bin UhSandbox
	rm -Rf ./include
	rm -Rf ./examples
	rm -Rf ./lib
	rm -Rf ./bin

clean:
	rm -f pandoDB-preview-*

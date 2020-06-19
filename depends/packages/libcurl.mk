package=libcurl
$(package)_version=7.70.0
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=ca2feeb8ef13368ce5d5e5849a5fd5e2dd4755fecf7d8f0cc94000a4206fb8e7
$(package)_dependencies=openssl

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-static --disable-ldap --disable-sspi --without-librtmp --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-smb
  $(package)_config_opts_mingw32=--without-ssl --with-schannel --with-random=/dev/urandom
  $(package)_config_opts_x86_64_mingw32=--target=x86_64-w64-mingw32
  $(package)_config_opts_i686_mingw32=--target=i686-w64-mingw32
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef


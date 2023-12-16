{
  pkgs ? import <nixpkgs> {}
, stdenv ? pkgs.stdenv
, debug ? true
, fetchMavenArtifact ? pkgs.fetchMavenArtifact
, jdk ? pkgs.jdk
, lib ? pkgs.lib
}:

   
let
  name = "aeron-cluster-cpp";

  aeron = pkgs.callPackage ./../pkgs/aeron { };
  aeron-cpp = pkgs.callPackage ./../pkgs/aeron-cpp { };
  aeron-archive-cpp = pkgs.callPackage ./../pkgs/aeron-archive-cpp { };
  aeron-cluster = pkgs.callPackage ./../pkgs/aeron-cluster.nix { };

  sbeAll_1_29_0 = fetchMavenArtifact {
    groupId = "uk.co.real-logic";
    version = "1.29.0";
    artifactId = "sbe-all";
    hash = "sha512-exklKS9MgOH369lyuv+5vAWRHt+Iwg/FmsWy8PsSMjenvjs8I2KA1VTa00pIXkw/YNqbUDBIWvS07b4mS8YdPQ==";
  };

  sbeAll = sbeAll_1_29_0;

  slf4j = fetchMavenArtifact {
    groupId = "org.slf4j";
    version = "2.0.9";
    artifactId = "slf4j-api";
    hash = "sha512-Bp5t3OeWF+N9YXWBIMfmg0juYvJVeBlIk3977DBY5GJEAm1/ahHpD7wVzUKIxLsazuTyQq9SHHIanmigXmTVJg==";
  };

  sbeTool = "uk.co.real_logic.sbe.SbeTool";

  add-links = ''
    ln --symbolic --force --target-directory=src \
      "${pkgs.ekam.src}/src/ekam/rules"

    mkdir --parents src/codecs

    ln --symbolic --force --target-directory=src/codecs/ \
      "${aeron-cpp.src}/aeron-cluster/src/main/resources/cluster/aeron-cluster-codecs.xml"

    cp --force --target-directory=src/codecs \
      "${aeron-cpp.src}/aeron-cluster/src/main/resources/cluster/aeron-cluster-mark-codecs.xml"

    # fixup for C++ reserved token
    sed -i 's/\"NULL\"/\"NULL_COMPONENT\"/g' src/codecs/aeron-cluster-mark-codecs.xml

    "${jdk}/bin/java" -cp "${sbeAll.jar}" -Dsbe.output.dir=src -Dsbe.target.language=Cpp -Dsbe.target.namespace=aeron.cluster.codecs "${sbeTool}" src/codecs/aeron-cluster-codecs.xml

    "${jdk}/bin/java" -cp "${sbeAll.jar}" -Dsbe.output.dir=src -Dsbe.target.language=Cpp -Dsbe.target.namespace=aeron.cluster.codecs.mark "${sbeTool}" src/codecs/aeron-cluster-mark-codecs.xml

    "${jdk}/bin/java" -cp "${sbeAll.jar}" -Dsbe.output.dir=java -Dsbe.target.language=Java -Dsbe.target.namespace=com.aeroncookbook.cluster.async.sbe "${sbeTool}" resources/messages.xml
  '';

in

stdenv.mkDerivation {

  inherit name;
  src = ./.;

  buildInputs = with pkgs; [
    aeron
    aeron-cpp
    aeron-archive-cpp
    capnproto
    openssl
    slf4j
    zlib
  ];

  nativeBuildInputs = with pkgs; [
    clang-tools
    ekam
    gtest
    jdk
    which
  ];

  propagatedBuildInputs = with pkgs; [
  ];

  CAPNPC_FLAGS = with pkgs; [
    "-I${capnproto}/include"
  ];

  SBE_JAR = sbeAll.jar;

  jdk = jdk;

  shellHook = add-links;

  buildPhase = ''
    ${add-links}
    make ${if debug then "debug" else "release"}
  '';

  installPhase = ''
    install --verbose -D --mode=644 \
      --target-directory="''${!outputLib}/lib" \
      lib${name}.a

    install --verbose -D --mode=644 \
      --target-directory="''${!outputInclude}/include/${name}" \
      src/*.capnp \
      src/*.capnp.h \
      tmp/*.capnp.h \
      src/*.h 
  '';
}

{
  pkgs ? import <nixpkgs> {}
, stdenv ? pkgs.stdenv
, debug ? true
, fetchMavenArtifact ? pkgs.fetchMavenArtifact
, jdk11 ? pkgs.jdk11
, lib ? pkgs.lib
}:

   
let
  name = "aeron-cluster-cpp";

  add-links = ''
    ln --symbolic --force --target-directory=src \
      "${pkgs.ekam.src}/src/ekam/rules"

    mkdir --parents src/codecs
    ln --symbolic --force --target-directory=src/codecs/ \
      "${pkgs.aeron.src}/aeron-cluster/src/main/resources/cluster/aeron-cluster-codecs.xml"
    ln --symbolic --force --target-directory=src/codecs \
      "${pkgs.aeron.src}/aeron-cluster/src/main/resources/cluster/aeron-cluster-mark-codecs.xml"

    java -cp "${sbeAll.jar}" -Dsbe.output.dir=src -Dsbe.target.language=Cpp -Dsbe.target.namespace=aeron.cluster.client uk.co.real_logic.sbe.SbeTool src/codecs/aeron-cluster-codecs.xml

    java -cp "${sbeAll.jar}" -Dsbe.output.dir=src -Dsbe.target.language=Cpp -Dsbe.target.namespace=aeron.cluster.client uk.co.real_logic.sbe.SbeTool src/codecs/aeron-cluster-mark-codecs.xml

  '';

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
 
in

stdenv.mkDerivation {

  inherit name;
  src = ./.;

  buildInputs = with pkgs; [
    aeron-cpp
    aeron-archive-cpp
    capnproto
    openssl
    zlib
  ];

  nativeBuildInputs = with pkgs; [
    clang-tools
    ekam
    gtest
    jdk11
    which
  ];

  propagatedBuildInputs = with pkgs; [
  ];

  CAPNPC_FLAGS = with pkgs; [
    "-I${capnproto}/include"
  ];

  SBE_JAR = sbeAll.jar;

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

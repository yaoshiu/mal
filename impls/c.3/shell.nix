{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  buildInputs = [
    pkgs.readline
    pkgs.pcre
  ];

  nativeBuildInputs = [
    pkgs.pkg-config
    pkgs.cmake
  ];
}

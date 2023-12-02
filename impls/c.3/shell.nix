{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  name = "nix-shell";
  buildInputs = [
    pkgs.readline
    pkgs.pcre
  ];

  nativeBuildInputs = [
    pkgs.clang-analyzer
    pkgs.pkg-config
    pkgs.cmake
    pkgs.valgrind
  ];
}

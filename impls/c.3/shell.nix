{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  name = "nix-shell";
  buildInputs = [
    pkgs.readline
    pkgs.pcre
  ];

  nativeBuildInputs = [
    pkgs.clang-analyzer
    pkgs.cmake
    pkgs.valgrind
  ];
}

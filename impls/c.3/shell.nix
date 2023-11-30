{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  name = "nix-shell";
  buildInputs = [
    pkgs.readline
    pkgs.pcre
  ];

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.valgrind
  ];
}

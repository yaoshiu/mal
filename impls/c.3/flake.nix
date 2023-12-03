{
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; }; in
      {
        devShells.default = pkgs.mkShell {
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
        };

        packages.default = pkgs.stdenv.mkDerivation {
          name = "mal-c.3";
          src = ./.;

          buildInputs = with pkgs; [
            readline
            pcre
          ];

          nativeBuildInputs = with pkgs; [
            pkg-config
            cmake
          ];
        };
      });
}

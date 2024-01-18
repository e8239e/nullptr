{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat.url = "github:edolstra/flake-compat";
    flake-compat.flake = false;
  };

  outputs = inputs@{ self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; };
      in {
        formatter = pkgs.nixfmt;
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            nixfmt
            valgrind # test for memory errors & leaks
            bear # extract compile commands for lsp
            clang-tools # lsp (clangd)
            ccls # lsp
            pkg-config
            cmake
            samurai
            fontforge
          ];
          buildInputs = (with pkgs; [ glfw libglvnd curlFull.dev libcpr ] ++ (with xorg; [ libX11 libXau libXdmcp ]));
        };
      });
}

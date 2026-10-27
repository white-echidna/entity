{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { nixpkgs, utils, ... }:
    utils.lib.eachSystem
      [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ]
      (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          devShells.default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.llvmPackages.libcxxStdenv;
              }
              {
                name = "entity";

                packages = with pkgs; [
                  llvmPackages.clang
                  llvmPackages.clang-tools

                  cmake
                  ninja

                  doctest
                  glm
                ];

                shellHook = ''
                  export CMAKE_GENERATOR="Ninja"
                  export CMAKE_EXPORT_COMPILE_COMMANDS="On"
                '';
              };

          packages.default = pkgs.callPackage ./default.nix { };
        }
      );
}

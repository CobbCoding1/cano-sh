{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};

  in {
    devShells.${system}.default = pkgs.mkShell {
      hardeningDisable = ["format" "fortify"];
      packages = with pkgs; [ gcc bear valgrind ncurses readline ];
    };
  };
}

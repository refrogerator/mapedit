with import <nixpkgs> {};
fastStdenv.mkDerivation {
	name = "mapedit";
	buildInputs = with pkgs; [ SDL2 pkg-config glm ];
}

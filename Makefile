
all: cspice_debc

go: cspice_debc
	for i in base_src/cspice/*[^_]?.c \
	; do \
	  ./cspice_debc "$${i}" \
	  | ./cspice_debc \
	  | ./cspice_debc - "$${i#base_}" \
        ; done

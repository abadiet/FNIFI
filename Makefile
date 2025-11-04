PLANTUML := plantuml

all: clean resources/architecture.png

resources/%.png: uml/%.plantuml
	@$(PLANTUML) $<
	@mv uml/$*.png resources/

clean:
	@rm -f resources/architecture.png

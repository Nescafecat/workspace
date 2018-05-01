cc = g++
prom = usestock0.out
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.cpp")
obj = $(src:%.cpp=%.o)

$(prom): $(obj)
	$(cc) -o $(prom) $(obj)

%.o: %.cpp $(deps)
	$(cc) -c $< -o $@

clean:
	rm -rf $(obj) $(prom)

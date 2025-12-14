
TEST_TARGET	:= test-minibpm
TEST_SOURCES	:= TestMiniBpm.cpp
MINIBPM_SOURCES	:= ../src/MiniBpm.cpp
MINIBPM_HEADERS	:= ../src/MiniBpm.h

CXXFLAGS	:= -g

TEST_LDFLAGS	:= $(LDFLAGS) -lboost_unit_test_framework

all: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) $(MINIBPM_SOURCES) $(MINIBPM_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SOURCES) $(TEST_LDFLAGS)


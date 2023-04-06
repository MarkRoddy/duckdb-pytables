
# Scalar Functions
def reverse(input):
    return input[::-1]

def scalar_throws_exception(input):
    raise Exception("This is an expected error")

# Table Functions
def table(input):
    for char in "a very long string":
        yield [char]

def table2(one_str_input, two_str_input, three_int_input):
    for c in one_str_input:
        yield [c]
    for c in two_str_input:
        yield [c]
    yield [str(three_int_input)]



#! /usr/bin/env python

import argparse

def convert_binary_to_array(binary_file, array_name):
    with open(binary_file, 'rb') as file:
        content = file.read()

    array_str = ', '.join(hex(byte) for byte in content)
    array_content = f"unsigned char {array_name}[] = {{ {array_str} }};"

    file.close()

    return array_content

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('binary_file', type=str, help='Path of the binary file')
    parser.add_argument('array_name', type=str, help='Name of the array')
    parser.add_argument('output_file', type=str, help='Path of the output file')

    args = parser.parse_args()

    c_array = convert_binary_to_array(args.binary_file, args.array_name)

    with open(args.output_file,mode='w') as output:
        output.write(c_array)
        output.close()


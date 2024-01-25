import os


# Unix-like cat function
# e.g. > cat('out', ['in0', 'in1'], path_to_in='./')
def cat(out_file_name, in_file_names, in_path='./', out_path='./'):
    with open(os.path.join(out_path, out_file_name), 'w') as out_file:
        for in_file_name in in_file_names:
            with open(os.path.join(in_path, in_file_name)) as in_file:
                for line in in_file:
                    if line.strip():
                        out_file.write(line)


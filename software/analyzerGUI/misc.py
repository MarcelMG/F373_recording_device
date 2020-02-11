import os

# replace "\\" in a filepath with "/" because somehow glob() inserts unconsistent slashes (?)
def fix_path(file_list):
    return [path.replace("\\", "/") for path in file_list]

import subprocess
import time
import os

program = str(input("Which program do you want to run?\n\tMPI\t\tPthread\t\tBoth\n> "))
times = int(input("How many times do you want to run?\n> "))
num_workers = str(input("How many processes/threads do you want to run?\n> "))
image = str(input("How large image size do you want to run? e.g. 800\n> "))
enable = str(input("Do you want to enable or disable the window?\n\tenable\t\tdisable\n> "))

# MPI_static_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_static 8 -2 2 -2 2 800 800 disable"
# MPI_dynamic_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_dynamic 8 -2 2 -2 2 800 800 disable"
# Pthread_static_run_command = os.path.join(".", "MS_pthread_static") + " " + num_workers + " -2 2 -2 2 800 800 disable"
# Pthread_dynamic_run_command = os.path.join(".", "MS_pthread_dynamic") + " " + num_workers + " -2 2 -2 2 800 800 disable"
# MPI_static_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_static 8 -2 2 -2 2 1000 1000 disable"
# MPI_dynamic_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_dynamic 8 -2 2 -2 2 1000 1000 disable"
# Pthread_static_run_command = os.path.join(".", "MS_pthread_static") + " " + num_workers + " -2 2 -2 2 1000 1000 disable"
# Pthread_dynamic_run_command = os.path.join(".", "MS_pthread_dynamic") + " " + num_workers + " -2 2 -2 2 1000 1000 disable"
MPI_static_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_static 8 -2 2 -2 2 " + image + " " + image + " " + enable
MPI_dynamic_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI_dynamic 8 -2 2 -2 2 " + image + " " + image + " " + enable
Pthread_static_run_command = os.path.join(".", "MS_pthread_static") + " " + num_workers + " -2 2 -2 2 " + image + " " + image + " " + enable
Pthread_dynamic_run_command = os.path.join(".", "MS_pthread_dynamic") + " " + num_workers + " -2 2 -2 2 " + image + " " + image + " " + enable
    
if(program == "MPI"):
    # MPI_file_name = str(raw_input("The MPI program that you want to execute (with extension):\n> ") or "MPI_mandelbrot_set.c") 
    # MPI_compile_command = "mpicc -o MS_MPI " + MPI_file_name + " -lX11"
    # subprocess.call(MPI_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(MPI_static_run_command)
        subprocess.call(MPI_static_run_command, shell=True)
        time.sleep(0.1)
        print(MPI_dynamic_run_command)
        subprocess.call(MPI_dynamic_run_command, shell=True)
        time.sleep(0.1)

elif (program == "Pthread"):
    # Pthread_file_name = str(raw_input("The Pthread program that you want to execute (with extension):\n> ") or "Pthread_mandelbrot_set.c") 
    # Pthread_compile_command = "gcc -o MS_Pthread " + Pthread_file_name + " -lpthread -lX11"
    # subprocess.call(Pthread_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(Pthread_static_run_command)
        subprocess.call(Pthread_static_run_command, shell=True)
        time.sleep(1)
        print(Pthread_dynamic_run_command)
        subprocess.call(Pthread_dynamic_run_command, shell=True)
        time.sleep(1)

elif (program == "Both"):
    # MPI_file_name = str(raw_input("The MPI program that you want to execute (with extension):\n> ") or "MPI_mandelbrot_set.c") 
    # Pthread_file_name = str(raw_input("The Pthread program that you want to execute (with extension):\n> ") or "Pthread_mandelbrot_set.c") 
    # MPI_compile_command = "mpicc -o MS_MPI " + MPI_file_name + " -lX11"
    # Pthread_compile_command = "gcc -o MS_Pthread " + Pthread_file_name + " -lpthread -lX11"
    # subprocess.call(MPI_compile_command, shell=True)
    # subprocess.call(Pthread_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        # print(MPI_run_command)
        # subprocess.call(MPI_run_command, shell=True)
        # time.sleep(0.1)
        # print(Pthread_run_command)
        # subprocess.call(Pthread_run_command, shell=True)
        # time.sleep(0.1)
        print(MPI_static_run_command)
        subprocess.call(MPI_static_run_command, shell=True)
        time.sleep(0.1)
        print(MPI_dynamic_run_command)
        subprocess.call(MPI_dynamic_run_command, shell=True)
        time.sleep(0.1)
        print(Pthread_static_run_command)
        subprocess.call(Pthread_static_run_command, shell=True)
        time.sleep(1)
        print(Pthread_dynamic_run_command)
        subprocess.call(Pthread_dynamic_run_command, shell=True)
        time.sleep(1)


import os,sys

exes = [x for x in os.listdir("./") if ".e" in x]

## seperate objects:
Tensor_exes = [ x for x in exes if "Tensor_" in x]
Storage_exes = [ x for x in exes if "Storage_" in x]
Bond_exes = [ x for x in exes if "Bond_" in x]
Accessor_exes = [ x for x in exes if "Accessor_" in x]

## generate output
for texe in Tensor_exes:
    output_name = (texe.split(".e")[0]).split("Tensor_")[-1] + ".cpp.out"
    os.system("./%s > Tensor/%s"%(texe,output_name))
    print(texe)
    print("================") 
    os.system("cat Tensor/%s"%(output_name))
    print("================") 

for texe in Storage_exes:
    output_name = (texe.split(".e")[0]).split("Storage_")[-1] + ".cpp.out"
    os.system("./%s > Storage/%s"%(texe,output_name))
    print(texe)
    print("================") 
    os.system("cat Storage/%s"%(output_name))
    print("================") 

for texe in Bond_exes:
    if(len(texe.split(".e")[0].split("Bond_"))>1):
        output_name = "combineBond_.cpp.out"
    else:
        output_name = (texe.split(".e")[0]).split("Bond_")[-1] + ".cpp.out"
    os.system("./%s > Bond/%s"%(texe,output_name))
    print(texe)
    print("================") 
    os.system("cat Bond/%s"%(output_name))
    print("================") 

for texe in Accessor_exes:
    output_name = (texe.split(".e")[0]).split("Accessor_")[-1] + ".cpp.out"
    os.system("./%s > Accessor/%s"%(texe,output_name))
    print(texe)
    print("================") 
    os.system("cat Accessor/%s"%(output_name))
    print("================") 

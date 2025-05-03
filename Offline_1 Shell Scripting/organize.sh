#!/bin/bash

rm -rf targets/
main_arg=0
for arg in "$@"
do
    if [ "${arg:0:1}" != "-" ]; then
        main_arg=$((main_arg + 1))
    fi
done
#echo "Number of mandatory arguments: $main_arg"
if [ $main_arg != 4 ]; then
    echo "Usage: $0 <submission_folder> <target_folder> <test_folder> <answer_folder> [options...]"
    echo
    echo "Mandatory Arguments (In Order):"
    echo "  1. Path of submission folder"
    echo "  2. Path of target folder"
    echo "  3. Path of test folder"
    echo "  4. Path of answer folder"
    echo
    echo "Optional Arguments (In Order):"
    echo "  -v         : Verbose (If provided, will print additional information while executing scripts)"
    echo
    echo "  -noexecute : If provided, will not execute main files"
    echo "               The CSV file won't have matched and not_matched columns"
    echo "               Code metrics might still be calculated"
    echo
    echo "  -nolc      : If provided, will not calculate line count in code metrics"
    echo "               The line_count column will not be included in the CSV file"
    echo
    echo "  -nocc      : If provided, will not calculate comment count in code metrics"
    echo "               The comment_count column will not be included in the CSV file"
    echo
    echo "  -nofc      : If provided, will not calculate function count in code metrics"
    echo "               The function_count column will not be included in the CSV file"
    echo
    kill -INT $$
fi

submission_folder="$1"
target_folder="$2"
test_folder="$3"
answer_folder="$4"

verbose=false
noexecute=false
nolc=false
nocc=false
nofc=false

shift 4 # removes the first 4 arguments

for arg in "$@"
do
    case $arg in
        -v)
            verbose=true
            ;;
        -noexecute)
            noexecute=true
            ;;
        -nolc)
            nolc=true
            ;;
        -nocc)
            nocc=true
            ;;
        -nofc)
            nofc=true
            ;;
        *)
            echo "Unknown option: $arg"
            kill -INT $$
            ;;
    esac
done

mkdir -p $target_folder
touch $target_folder/result.csv
header="student_id,student_name,language"

if [ "$noexecute" = false ]; then
    header+=",matched,not_matched"
fi

if [ "$nolc" = false ]; then
    header+=",line_count"
fi

if [ "$nocc" = false ]; then
    header+=",comment_count"
fi

if [ "$nofc" = false ]; then
    header+=",function_count"
fi

echo "$header" >> $target_folder/result.csv


for i in $submission_folder/*.zip
do
	unzip "$i" -d answers/primary/ > /dev/null
done

run()
{
fileName="$1"
folderName="$2"
matched=0
not_matched=0
#count no of files at first
for((j=1;j<6;j++))
do 
	./"$fileName" < "$test_folder/test$j.txt" > "$folderName/out$j.txt"
	if diff -q "$folderName/out$j.txt" "$answer_folder/ans$j.txt" > /dev/null
	then
		matched=$((matched + 1))
	else
		not_matched=$((not_matched + 1))
	fi
done
echo "$matched $not_matched"
}

runPy()
{
fileName="$1"
folderName="$2"
matched=0
not_matched=0
#count no of files at first
for((j=1;j<6;j++))
do 
	python3 "$fileName" < "$test_folder/test$j.txt" > "$folderName/out$j.txt"
	if diff -q "$folderName/out$j.txt" "$answer_folder/ans$j.txt" > /dev/null
	then
		matched=$((matched + 1))
	else
		not_matched=$((not_matched + 1))
	fi
done
echo "$matched $not_matched"
}

runJava()
{
folderName="$1"
matched=0
not_matched=0
#count no of files at first
for((j=1;j<6;j++))
do 
	java -cp "$folderName" Main < "$test_folder/test$j.txt" > "$folderName/out$j.txt"
	if diff -q "$folderName/out$j.txt" "$answer_folder/ans$j.txt" > /dev/null
	then
		matched=$((matched + 1))
	else
		not_matched=$((not_matched + 1))
	fi
done
echo "$matched $not_matched"
}


for i in answers/primary/*; do
    if [ -d "$i" ]; then
    	studentName=$(basename "$i" | cut -d'_' -f1-1)
        studentId=$(echo "$i" | rev | cut -c 1-7 | rev)
        #echo "Student ID: $studentId $studentName"
        language=""
        fileName=$(find "$i" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.java" -o -name "*.py" \))
        # echo "Found file: $fileName"

        ext="${fileName##*.}"  

        case "$ext" in
            c)
                if [ "$verbose" == true ]; then
                    echo "Organizing files of $studentId"
                fi
                mkdir -p $target_folder/C/"$studentId"
                cp "$fileName" $target_folder/C/"$studentId"/
                mv $target_folder/C/"$studentId"/* $target_folder/C/"$studentId"/main.c > /dev/null
            	language="C"
                if [ "$nocc" == false ]; then
                    comment_count=$(grep -c "//" $target_folder/C/"$studentId"/main.c)
                fi
                if [ "$nolc" == false ]; then
                    line_count=$(grep -c '' $target_folder/C/"$studentId"/main.c)
                fi
                if [ "$nofc" == false ]; then
                    function_count=$(grep -E -c '^\s*[a-zA-Z][a-zA-Z0-9_]*\s+[*]*\s*[a-zA-Z][a-zA-Z0-9_]*\s*\(.*\)\s*\{' $target_folder/C/"$studentId"/main.c)
                fi
                if [ "$noexecute" == false ]; then
                    if [ "$verbose" == true ]; then
                        echo "Executing files of $studentId"
                    fi
                    gcc $target_folder/C/"$studentId"/main.c -o $target_folder/C/"$studentId"/main.out
                    read matched not_matched < <(run "$target_folder/C/$studentId/main.out" "$target_folder/C/$studentId")
                fi
                ;;
            cpp)
                if [  "$verbose" == true ]; then
                    echo "Organizing files of $studentId"
                fi
                mkdir -p $target_folder/C++/"$studentId"
                cp "$fileName" $target_folder/C++/"$studentId"/
                mv $target_folder/C++/"$studentId"/* $target_folder/C++/"$studentId"/main.cpp > /dev/null
            	language=C++
                if [ "$nocc" == false ]; then
                    comment_count=$(grep -c "//" $target_folder/C++/"$studentId"/main.cpp)
                fi
                if [ "$nolc" == false ]; then
                    line_count=$(grep -c '' $target_folder/C++/"$studentId"/main.cpp)
                fi
                if [ "$nofc" == false ]; then
                    function_count=$(grep -E -c '^\s*[a-zA-Z][a-zA-Z0-9_]*\s+[*]*\s*[a-zA-Z][a-zA-Z0-9_]*\s*\(.*\)\s*\{' $target_folder/C++/"$studentId"/main.cpp)
                fi
                if [ "$noexecute" == false ]; then
                    if [ "$verbose" == true ]; then
                        echo "Executing files of $studentId"
                    fi
                    g++ $target_folder/C++/"$studentId"/main.cpp -o $target_folder/C++/"$studentId"/main.out
                    read matched not_matched < <(run "$target_folder/C++/$studentId/main.out" "$target_folder/C++/$studentId")
                fi
                ;;
            java)
                if [  "$verbose" == true ]; then
                    echo "Organizing files of $studentId"
                fi
                mkdir -p $target_folder/Java/"$studentId"
                cp "$fileName" $target_folder/Java/"$studentId"/
                mv $target_folder/Java/"$studentId"/* $target_folder/Java/"$studentId"/Main.java > /dev/null 2>&1
            	language=Java
                if [ "$nocc" == false ]; then
                    comment_count=$(grep -c "//" $target_folder/Java/"$studentId"/Main.java)
                fi
                if [ "$nolc" == false ]; then
                    line_count=$(grep -c '' $target_folder/Java/"$studentId"/Main.java)
                fi
                if [ "$nofc" == false ]; then
                    function_count=$(grep -E -c '^\s*(public|private|protected)?\s*(static\s+)?[a-zA-Z]+\s+[a-zA-Z][a-zA-Z0-9_]*\s*\(.*\)\s*\{' $target_folder/Java/"$studentId"/Main.java)
                fi
                if [ "$noexecute" == false ]; then
                    if [ "$verbose" == true ]; then
                        echo "Executing files of $studentId"
                    fi
                    javac $target_folder/Java/"$studentId"/Main.java
                    read matched not_matched < <(runJava "$target_folder/Java/$studentId")
                fi
                ;;
            py)
                if [  "$verbose" == true ]; then
                    echo "Organizing files of $studentId"
                fi
                mkdir -p $target_folder/Python/"$studentId"
                cp "$fileName" $target_folder/Python/"$studentId"/
                mv $target_folder/Python/"$studentId"/* $target_folder/Python/"$studentId"/main.py > /dev/null 2>&1
            	language=Python
                if [ "$nocc" == false ]; then
                    comment_count=$(grep -c "#" $target_folder/Python/"$studentId"/main.py)
                fi
                if [ "$nolc" == false ]; then
                    line_count=$(grep -c '' $target_folder/Python/"$studentId"/main.py)
                fi
                if [ "$nofc" == false ]; then
                    function_count=$(grep -c "def " $target_folder/Python/"$studentId"/main.py)
                fi
                if [ "$noexecute" == false ]; then
                    if [ "$verbose" == true ]; then
                        echo "Executing files of $studentId"
                    fi
                    read matched not_matched < <(runPy "$target_folder/Python/$studentId/main.py" "$target_folder/Python/$studentId")
                fi
                ;;
        esac
        #echo $studentId,"$studentName",$language,$matched,$not_matched,$line_count,$comment_count >> $target_folder/result.csv
        row="$studentId,\"$studentName\",$language"

        if [ "$noexecute" = false ]; then
            row+=",${matched},${not_matched}"
        fi

        if [ "$nolc" = false ]; then
            row+=",${line_count}"
        fi

        if [ "$nocc" = false ]; then
            row+=",${comment_count}"
        fi

        if [ "$nofc" = false ]; then
            row+=",${function_count}"
        fi

        echo "$row" >> "$target_folder/result.csv"

    fi
done

if [ "$verbose" == true ]; then
    echo "All submissions processed successfully"
fi

rm -rf answers/primary/





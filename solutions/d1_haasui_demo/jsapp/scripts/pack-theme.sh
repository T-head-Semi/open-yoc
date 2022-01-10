if [ -z "$1" ]; then
    echo "should give output path"
    exit 1
fi


# compile theme1
sed -ie "s/theme-custom2/theme-custom1/g" src/app.json
aiot-cli build

THEME01_PATH="$1/theme1"
if [ -e $THEME01_PATH ]; then
    rm -rf $THEME01_PATH
fi

cp -r .falcon_ $THEME01_PATH

echo "pack success $THEME01_PATH"

# compile theme2
sed -ie "s/theme-custom1/theme-custom2/g" src/app.json
aiot-cli build

THEME02_PATH="$1/theme2"
if [ -e $THEME02_PATH ]; then
    rm -rf $THEME02_PATH
fi

cp -r .falcon_ $THEME02_PATH

echo "pack success $THEME02_PATH"

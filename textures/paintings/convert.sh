for image in *.webp; do
  name=`basename -s.webp "$image"`
  convert "$image" "${name}.png"
done

import pandas as pd


with open("pytohn_scripts/colors.txt") as f:
    data = f.read()
data = data.split("\n")
data = [i.split(" - ") for i in data]
print(data)

# Create a DataFrame for markup generation

df = pd.DataFrame(data, columns=['Hex', 'Description'])


# Convert to RGB565

def hex_to_rgb565(hex_color):

    r, g, b = int(hex_color[0:2], 16), int(
        hex_color[2:4], 16), int(hex_color[4:6], 16)

    rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

    return f"0x{rgb565:04X}"


df['RGB565'] = df['Hex'].apply(hex_to_rgb565)


cpp_rgb565_list = "uint16_t colorTable[] = {\n"

cpp_rgb565_list += ",\n".join([f"    {row['RGB565']}" for _,
                              row in df.iterrows()])

cpp_rgb565_list += "\n};"


# Create enhanced HTML markup

enhanced_markup_content = """

<html>

<head>

<style>

     body {

        font-family: Arial, sans-serif;

        line-height: 1.6;

        margin: 20px;

        background-color: 0;
        color: white;

    }

    .color-table {

        border-collapse: collapse;

        width: 100%;

        margin-top: 20px;

    }

    .color-table th, .color-table td{

        border: 1px solid #ddd;

        padding: 8px;

    }
    .color-table :first-child{

        border: 1px solid #ddd;

        padding: 0px;

    }

    .color-table th {

        padding-top: 12px;

        padding-bottom: 12px;

        text-align: left;

        background-color: #4CAF50;

        color: white;

    }

    .color-box {

        width: 100%;

        height: 50px;

        display: inline-block;

        border: 1px solid #ccc;

    }

</style>

</head>

<body>

<h1>Enhanced Color Table</h1>

<table class="color-table">

    <thead>

        <tr>

            <th>Color</th>

            <th>Hex</th>

            <th>Description</th>

            <th>RGB565</th>

        </tr>

    </thead>

    <tbody>

"""


for _, row in df.iterrows():

    enhanced_markup_content += f"""

        <tr>

            <td><span class="color-box" style="background-color: #{row['Hex']};"></span></td>

            <td>#{row['Hex']}</td>

            <td>{row['Description']}</td>

            <td>{row['RGB565']}</td>

        </tr>

    """


enhanced_markup_content += """

    </tbody>

</table>

</body>

</html>

"""


# Save enhanced HTML file

enhanced_markup_file_path = 'pytohn_scripts/enhanced_color_table.html'

with open(enhanced_markup_file_path, 'w') as f:

    f.write(enhanced_markup_content)


# Save C++ file

cpp_file_path = 'pytohn_scripts/color_table.cpp'

with open(cpp_file_path, 'w') as f:

    f.write(cpp_rgb565_list)

# Hệ vật liệu M7

AssetData chứa một danh sách MaterialData. Mỗi submesh tham chiếu một phần tử qua materialIndex. Renderer kết hợp màu nền, UV, diffuse map, màu phản xạ Ks và độ bóng Ns. Base color của editor và màu Kd nhập được xử lý theo quy ước màu hiển thị của dự án. Texture màu sRGB được GPU giải mã khi lấy mẫu. Ks là hệ số tuyến tính. Khung hình hiện dùng vật liệu opaque.

- Với OBJ, ModelLoader đọc Kd, Ks, Ns và map_Kd qua Assimp. Nhựa phím, vỏ kim loại và mặt màn hình giữ đúng chỉ số vật liệu và UV do Blender xuất.
- Với mô hình thủ tục, bộ chuyển đổi fromFurnitureModel gán màu từ từng phần. Gỗ dùng Ks = 0,12 và Ns = 26. Đệm vải dùng 0,025 và 8. Bình gốm dùng 0,48 và 72. Chân/khớp đèn kim loại dùng 0,56 và 90, chụp đèn dùng 0,10 và 24.
- Tường giữ màu sơn kem từ RoomSettings, Ks = 0,045, Ns = 20. Sàn dùng cùng nền phản xạ dịu, biến thiên tuần hoàn nhỏ theo tọa độ mét để gợi vân sồi.
- Thiếu khai báo texture thì dùng màu nền. File texture được khai báo nhưng không đọc được sẽ dùng checker dự phòng của TextureManager. Inspector đọc dữ liệu thật của từng vật liệu.
- SceneObject tham chiếu asset theo ID. Chỉnh ánh sáng tác động lên cảnh. Shininess scale thuộc SceneObject, nhân Ns theo từng instance và mặc định là 1. Bản sao có hệ số riêng. Thumbnail chứa weak_ptr nhận diện đúng thế hệ GpuAsset kể cả khi allocator tái dùng địa chỉ. Texture cũ được giải phóng sau khi tạo ảnh thay thế thành công.

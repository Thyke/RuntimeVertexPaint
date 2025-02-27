# Runtime Vertex Paint System

*Read this in other languages: [English](#runtime-vertex-paint-system-english), [Turkish](#runtime-vertex-paint-system-türkçe)*

## Runtime Vertex Paint System (English)

This system provides a Blueprint Function Library for modifying static mesh vertex colors at runtime in Unreal Engine. It can be used for dynamic vertex painting in-game, altering surface appearance, showing damage, snow accumulation, mud, blood stains, and other similar effects.

### Features

- Paint individual vertices or groups of vertices
- Various painting shapes (point, sphere, box, cylinder)
- Different color blending modes (replace, add, multiply, linear interpolation)
- Falloff for smooth color transitions toward edges
- Full LOD support (paint on a single LOD or all LOD levels)

### Installation

1. Copy the plugin to your project's "Plugins" folder. Create the folder if it does not exist.
2. Start Unreal Engine (or your game).
3. Ensure "Vertex Paint" plugin is enabled from Editor > Plugins menu.
4. Access the Blueprint functions under the "Vertex" category to use the plugin.

### Usage

#### Basic Painting Operations

```cpp
// Paint a single vertex
UVertexBlueprintFunctionLibrary::PaintVertexColorByIndex(StaticMeshComponent, FLinearColor::Red, 0);

// Paint vertices within a sphere
UVertexBlueprintFunctionLibrary::PaintMeshRegion(
    StaticMeshComponent,           // Mesh component to paint
    EVertexPaintShape::Sphere,     // Shape (Sphere, Box, Cylinder, Point)
    HitLocation,                   // World location
    FVector(100.0f, 100.0f, 100.0f), // Dimensions (X=radius)
    FRotator::ZeroRotator,         // Rotation
    FLinearColor::Red,             // Color
    EVertexColorBlendMode::Add,    // Blend mode
    0.5f,                          // Blend strength
    0.5f                           // Falloff value
);

// Reset all vertex colors
UVertexBlueprintFunctionLibrary::ResetVertexColors(StaticMeshComponent);
```

#### Advanced Usage

```cpp
// Complex painting operation
FVertexPaintParameters PaintParams;
PaintParams.PaintShape = EVertexPaintShape::Box;
PaintParams.Location = BoxCenter;
PaintParams.Dimensions = FVector(200.0f, 100.0f, 150.0f);
PaintParams.Rotation = BoxRotation;
PaintParams.Color = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f); // Blue
PaintParams.BlendMode = EVertexColorBlendMode::Multiply;
PaintParams.BlendStrength = 0.75f;
PaintParams.Falloff = 0.2f;
PaintParams.bApplyToAllLODs = true;
UVertexBlueprintFunctionLibrary::PaintMeshWithParameters(StaticMeshComponent, PaintParams);
```

#### Blueprint Usage

You can easily use the vertex painting functions in Blueprints:

1. Search for functions under the "Vertex" category in your Blueprint
2. Select the desired function (e.g., "Paint Mesh Region")
3. Set the required parameters
4. For example, to paint where the player clicks, use Line Trace and use the hit location as the painting location

### Material Usage

**IMPORTANT:** To see vertex colors, you need to use a **Vertex Color** node in your material:

1. Add a Vertex Color node in the Material Editor.
2. Connect the output to the desired material property (e.g., Base Color).
3. If you want to use just a specific channel, you can use the R, G, B, or A outputs of the Vertex Color node separately.

![Material Setup](/images/VertexNode.png)

#### Example Material Uses:

- **Damage Effect**: Use Vertex Color's R channel as damage amount and lerp from black to red.
- **Snow Accumulation**: Use Vertex Color's G channel as snow amount and blend in snow texture.
- **Mud/Dirt**: Use Vertex Color's B channel as mud/dirt amount affecting roughness and normal maps.
- **As Mask**: Use Alpha channel as a mask to transition between different materials.

### Performance Recommendations

- Very large meshes (10,000+ vertices) may cause performance issues. Consider breaking them into smaller pieces if possible.
- Only use `bApplyToAllLODs` when necessary, as it processes each LOD level.
- Use optimized shaders and materials for runtime performance.
- Avoid painting many meshes simultaneously.

### Note:

Changes made at runtime are only valid for the current session and will be lost when the game is closed. For persistent changes, you would need to implement a SaveGame mechanism to separately store the vertex color data.

### Examples

#### Creating Footprints

```cpp
// Called when player takes a step
void APlayerCharacter::CreateFootprint()
{
    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0, 0, 100); // Line trace toward ground
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility))
    {
        if (UStaticMeshComponent* FloorMesh = Cast<UStaticMeshComponent>(HitResult.Component))
        {
            // Create footprint
            FVertexPaintParameters Params;
            Params.PaintShape = EVertexPaintShape::Sphere;
            Params.Location = HitResult.Location;
            Params.Dimensions = FVector(40.0f, 40.0f, 10.0f); // Footprint size
            Params.Color = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black
            Params.BlendMode = EVertexColorBlendMode::Multiply;
            Params.BlendStrength = 0.8f;
            Params.Falloff = 0.7f;
            
            UVertexBlueprintFunctionLibrary::PaintMeshWithParameters(FloorMesh, Params);
        }
    }
}
```

#### Melting Snow

```cpp
// When a fireball or other heat source touches snow
void AMeltableSnow::OnHeatSourceOverlap(UPrimitiveComponent* HeatSource)
{
    FVector HeatLocation = HeatSource->GetComponentLocation();
    float HeatRadius = Cast<USphereComponent>(HeatSource)->GetScaledSphereRadius() * 2.0f;
    
    // Melt the snow (if snow is white and melting is black)
    UVertexBlueprintFunctionLibrary::PaintMeshRegion(
        SnowMeshComponent,
        EVertexPaintShape::Sphere,
        HeatLocation,
        FVector(HeatRadius, HeatRadius, HeatRadius),
        FRotator::ZeroRotator,
        FLinearColor::Black,
        EVertexColorBlendMode::Replace,
        0.8f,
        0.3f
    );
}
```

### License

This project is licensed under the MIT License - see the LICENSE file for details.

### Technical Details

#### Vertex Color Format

In Unreal Engine, vertex colors are stored in RGBA 8-bit format. This means values between 0-255 for each channel. Most functions use FLinearColor (0.0-1.0 float values) which are internally converted to 8-bit format.

#### LOD and Render State

When modifying vertex colors, the VertexColorBuffer is recreated and notified to the render thread. This has a performance cost, especially for large meshes with many vertices. Therefore, it's recommended to batch vertex color changes when possible.

#### Multi-LOD Support

The system supports painting all LOD levels simultaneously. You can use this feature with the `bApplyToAllLODs` parameter. However, be aware that LOD levels may have different vertex counts, which might not give expected results in some cases.

#### Forward Renderer vs Deferred Renderer

Unreal Engine's Deferred Renderer fully supports vertex colors. However, if you're using Forward Renderer (e.g., in VR or mobile projects), you may need additional settings for vertex color usage.

---

## Runtime Vertex Paint System (Türkçe)

Bu sistem, Unreal Engine'de statik mesh vertex renklerini çalışma zamanında değiştirmek için kullanılabilen bir Blueprint Function Library sağlar. Oyun içinde dinamik vertex boyama, yüzeylerin görünümünü değiştirme, hasar gösterme, kar birikimi, çamur, kan lekeleri gibi efektler için kullanılabilir.

### Özellikler

- Tek bir vertex'i veya vertex gruplarını boyama
- Çeşitli şekillerde boyama (nokta, küre, kutu, silindir)
- Farklı renk karıştırma modları (değiştirme, ekleme, çarpma, doğrusal interpolasyon)
- Kenarlara doğru azalan renk geçişi (falloff)
- Tam LOD desteği (tek bir LOD veya tüm LOD seviyelerinde boyama)

### Kurulum

1. Plugin'i projenizin "Plugins" klasörüne kopyalayın. Klasör yoksa oluşturun.
2. Unreal Engine'i (veya oyununuzu) başlatın.
3. Editor > Plugins menüsünden "Vertex Paint" plugin'inin etkin olduğundan emin olun.
4. Plugin'i kullanmak için "Vertex" kategorisi altındaki Blueprint fonksiyonlarına erişebilirsiniz.

### Kullanım

#### Temel Boyama İşlemleri

```cpp
// Tek bir vertex'i boyama
UVertexBlueprintFunctionLibrary::PaintVertexColorByIndex(StaticMeshComponent, FLinearColor::Red, 0);

// Bir küre içindeki vertex'leri boyama
UVertexBlueprintFunctionLibrary::PaintMeshRegion(
    StaticMeshComponent,           // Boyanacak mesh bileşeni
    EVertexPaintShape::Sphere,     // Boyama şekli (Sphere, Box, Cylinder, Point)
    HitLocation,                   // Dünya konumu
    FVector(100.0f, 100.0f, 100.0f), // Boyutlar (X=yarıçap)
    FRotator::ZeroRotator,         // Rotasyon
    FLinearColor::Red,             // Renk
    EVertexColorBlendMode::Add,    // Karıştırma modu
    0.5f,                          // Karıştırma şiddeti
    0.5f                           // Kenar geçiş değeri
);

// Tüm vertex renklerini sıfırlama
UVertexBlueprintFunctionLibrary::ResetVertexColors(StaticMeshComponent);
```

#### İleri Düzey Kullanım

```cpp
// Karmaşık bir boyama işlemi
FVertexPaintParameters PaintParams;
PaintParams.PaintShape = EVertexPaintShape::Box;
PaintParams.Location = BoxCenter;
PaintParams.Dimensions = FVector(200.0f, 100.0f, 150.0f);
PaintParams.Rotation = BoxRotation;
PaintParams.Color = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f); // Mavi
PaintParams.BlendMode = EVertexColorBlendMode::Multiply;
PaintParams.BlendStrength = 0.75f;
PaintParams.Falloff = 0.2f;
PaintParams.bApplyToAllLODs = true;
UVertexBlueprintFunctionLibrary::PaintMeshWithParameters(StaticMeshComponent, PaintParams);
```

#### Blueprint Kullanımı

Vertex boyama işlevlerini Blueprint'lerde kolayca kullanabilirsiniz:

1. Blueprint içinde "Vertex" kategorisi altındaki fonksiyonları arayın
2. İstediğiniz fonksiyonu seçin (örn. "Paint Mesh Region")
3. Gerekli parametreleri belirleyin
4. Örnek olarak, oyuncunun tıkladığı yerde boyama yapmak için Line Trace fonksiyonunu kullanıp hit location'ı boyama konumu olarak kullanabilirsiniz

### Material Kullanımı

**ÖNEMLİ:** Vertex renklerini görmek için material'da **Vertex Color** node'unu kullanmanız gerekir:

1. Material Editor'da Vertex Color node'u ekleyin.
2. Bu node'un çıkışını istediğiniz materyal özelliğine (örn. Base Color) bağlayın.
3. Eğer sadece belirli bir kanalı kullanmak istiyorsanız, Vertex Color node'unun R, G, B veya A çıkışlarını ayrı ayrı kullanabilirsiniz.

![Material Setup](/images/VertexNode.png)

#### Örnek Material Kullanımları:

- **Hasar Efekti**: Vertex Color'ın R kanalını hasar miktarı olarak kullanıp, siyahtan kırmızıya bir lerp yapabilirsiniz.
- **Kar Birikimi**: Vertex Color'ın G kanalını kar miktarı olarak kullanıp, kar dokusunu blend edebilirsiniz.
- **Çamur/Kir**: Vertex Color'ın B kanalını çamur/kir miktarı olarak kullanıp, roughness ve normal map'i etkileyebilirsiniz.
- **Maske Olarak**: Alpha kanalını farklı materyaller arasında geçiş için maske olarak kullanabilirsiniz.

### Performans Önerileri

- Çok büyük mesh'lerde (10,000+ vertex) performans sorunları yaşanabilir. Mümkünse mesh'leri daha küçük parçalara bölün.
- `bApplyToAllLODs` seçeneğini sadece gerektiğinde kullanın, çünkü her LOD seviyesi için işlem yapılır.
- Runtime performansı için optimize edilmiş shader ve materyal kullanın.
- Çok sayıda mesh'i aynı anda boyamaktan kaçının.

### Not:

Runtime'da yapılan değişiklikler sadece o oturum için geçerlidir ve oyun kapatıldığında kaybolur. Kalıcı değişiklikler için SaveGame mekanizması ile vertex renk verilerini ayrıca kaydetmeniz gerekir.

### Örnekler

#### Adım Izleri Oluşturma

```cpp
// Oyuncu her adım attığında çağrılır
void APlayerCharacter::CreateFootprint()
{
    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0, 0, 100); // Zemine doğru line trace
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility))
    {
        if (UStaticMeshComponent* FloorMesh = Cast<UStaticMeshComponent>(HitResult.Component))
        {
            // Footprint oluştur
            FVertexPaintParameters Params;
            Params.PaintShape = EVertexPaintShape::Sphere;
            Params.Location = HitResult.Location;
            Params.Dimensions = FVector(40.0f, 40.0f, 10.0f); // Ayak izi boyutu
            Params.Color = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // Siyah
            Params.BlendMode = EVertexColorBlendMode::Multiply;
            Params.BlendStrength = 0.8f;
            Params.Falloff = 0.7f;
            
            UVertexBlueprintFunctionLibrary::PaintMeshWithParameters(FloorMesh, Params);
        }
    }
}
```

#### Kar Eritme

```cpp
// Ateş topu veya başka bir sıcak obje kar ile temas ettiğinde
void AMeltableSnow::OnHeatSourceOverlap(UPrimitiveComponent* HeatSource)
{
    FVector HeatLocation = HeatSource->GetComponentLocation();
    float HeatRadius = Cast<USphereComponent>(HeatSource)->GetScaledSphereRadius() * 2.0f;
    
    // Karı erit (kar beyaz, erime siyah ise)
    UVertexBlueprintFunctionLibrary::PaintMeshRegion(
        SnowMeshComponent,
        EVertexPaintShape::Sphere,
        HeatLocation,
        FVector(HeatRadius, HeatRadius, HeatRadius),
        FRotator::ZeroRotator,
        FLinearColor::Black,
        EVertexColorBlendMode::Replace,
        0.8f,
        0.3f
    );
}
```

### Lisans

Bu proje MIT lisansı ile lisanslanmıştır. Detaylar için LICENSE dosyasına bakın.

### Teknik Detaylar

#### Vertex Color Formatı

Unreal Engine'de vertex color'lar RGBA 8-bit formatında saklanır. Bu, her kanal için 0-255 arası değerler anlamına gelir. Fonksiyonların çoğu FLinearColor (0.0-1.0 float değerleri) kullanır ve bu değerler dahili olarak 8-bit formata dönüştürülür.

#### LOD ve Render State

Vertex renklerini değiştirdiğinizde, VertexColorBuffer yeniden oluşturulur ve render thread'e bildirilir. Bu, özellikle çok sayıda vertex içeren büyük mesh'ler için bir performans maliyeti getirebilir. Bu nedenle, vertex renk değişikliklerini mümkün olduğunca toplu olarak yapmak önerilir.

#### Çoklu LOD Desteği

Sistem, tüm LOD seviyelerini aynı anda boyamayı destekler. Bu özelliği `bApplyToAllLODs` parametresi ile kullanabilirsiniz. Ancak, LOD seviyeleri farklı vertex sayılarına sahip olabileceğinden, bazı durumlarda beklenen sonuçları alamayabilirsiniz.

#### Forward Renderer vs Deferred Renderer

Unreal Engine'in Deferred Renderer'ı vertex color'ları tam olarak destekler. Ancak, Forward Renderer kullanıyorsanız (örn. VR veya mobil projelerde), vertex color kullanımı için ek ayarlar gerekebilir.